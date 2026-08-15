/*
   DDS, a bridge double dummy solver.

   Reader for recorded DDS workloads (JSON Lines). See recording.hpp.

   See LICENSE and README.
*/

#include "recording.hpp"

#include <cstdio>
#include <fstream>
#include <set>

namespace dds_replay {

namespace {

// ---------------------------------------------------------------------------
// A small recursive-descent JSON parser, sufficient for the recorder's output.
// ---------------------------------------------------------------------------

struct Parser
{
  const std::string& s;
  size_t i = 0;
  std::string error;

  explicit Parser(const std::string& text) : s(text) {}

  auto skip_ws() -> void
  {
    while (i < s.size() &&
           (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r'))
      ++i;
  }

  auto fail(const char* what) -> bool
  {
    if (error.empty())
      error = std::string(what) + " at offset " + std::to_string(i);
    return false;
  }

  auto literal(const char* lit) -> bool
  {
    const size_t n = std::char_traits<char>::length(lit);
    if (s.compare(i, n, lit) != 0)
      return fail("bad literal");
    i += n;
    return true;
  }

  auto parse_string(std::string& out) -> bool
  {
    if (i >= s.size() || s[i] != '"')
      return fail("expected string");
    ++i;
    out.clear();
    while (i < s.size() && s[i] != '"') {
      char c = s[i++];
      if (c != '\\') {
        out += c;
        continue;
      }
      if (i >= s.size())
        return fail("truncated escape");
      const char e = s[i++];
      switch (e) {
        case 'n': out += '\n'; break;
        case 't': out += '\t'; break;
        case 'r': out += '\r'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case '/': out += '/';  break;
        case '"': out += '"';  break;
        case '\\': out += '\\'; break;
        case 'u': {
          if (i + 4 > s.size())
            return fail("truncated \\u escape");
          const unsigned cp = static_cast<unsigned>(
            std::strtoul(s.substr(i, 4).c_str(), nullptr, 16));
          i += 4;
          // The recorder only ever writes ASCII (PBN strings and short tags),
          // so a minimal UTF-8 encoding of the BMP range is enough here.
          if (cp < 0x80) {
            out += static_cast<char>(cp);
          } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
          } else {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
          }
          break;
        }
        default: return fail("unknown escape");
      }
    }
    if (i >= s.size())
      return fail("unterminated string");
    ++i;  // closing quote
    return true;
  }

  auto parse_value(JsonValue& v) -> bool
  {
    skip_ws();
    if (i >= s.size())
      return fail("unexpected end");

    const char c = s[i];
    if (c == '{') {
      ++i;
      v.kind = JsonValue::Kind::Object;
      skip_ws();
      if (i < s.size() && s[i] == '}') { ++i; return true; }
      for (;;) {
        skip_ws();
        std::string key;
        if (!parse_string(key))
          return false;
        skip_ws();
        if (i >= s.size() || s[i] != ':')
          return fail("expected ':'");
        ++i;
        JsonValue child;
        if (!parse_value(child))
          return false;
        v.members.emplace_back(std::move(key), std::move(child));
        skip_ws();
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        if (i < s.size() && s[i] == '}') { ++i; return true; }
        return fail("expected ',' or '}'");
      }
    }
    if (c == '[') {
      ++i;
      v.kind = JsonValue::Kind::Array;
      skip_ws();
      if (i < s.size() && s[i] == ']') { ++i; return true; }
      for (;;) {
        JsonValue child;
        if (!parse_value(child))
          return false;
        v.items.push_back(std::move(child));
        skip_ws();
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        if (i < s.size() && s[i] == ']') { ++i; return true; }
        return fail("expected ',' or ']'");
      }
    }
    if (c == '"') {
      v.kind = JsonValue::Kind::String;
      return parse_string(v.text);
    }
    if (c == 't') { v.kind = JsonValue::Kind::Bool; v.boolean = true;  return literal("true"); }
    if (c == 'f') { v.kind = JsonValue::Kind::Bool; v.boolean = false; return literal("false"); }
    if (c == 'n') { v.kind = JsonValue::Kind::Null; return literal("null"); }

    // number
    const size_t start = i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
    while (i < s.size() &&
           ((s[i] >= '0' && s[i] <= '9') || s[i] == '.' ||
            s[i] == 'e' || s[i] == 'E' || s[i] == '-' || s[i] == '+'))
      ++i;
    if (i == start)
      return fail("expected value");
    v.kind = JsonValue::Kind::Number;
    v.number = std::strtod(s.substr(start, i - start).c_str(), nullptr);
    return true;
  }
};

auto to_int_vector(const JsonValue* v) -> std::vector<int>
{
  std::vector<int> out;
  if (v == nullptr || v->kind != JsonValue::Kind::Array)
    return out;
  for (const auto& item : v->items)
    if (item.kind == JsonValue::Kind::Number)
      out.push_back(static_cast<int>(item.number));
  return out;
}

auto to_result_map(const JsonValue* v) -> ResultMap
{
  ResultMap out;
  if (v == nullptr || v->kind != JsonValue::Kind::Object)
    return out;
  for (const auto& [key, values] : v->members)
    out[key] = to_int_vector(&values);
  return out;
}

}  // namespace

namespace {

auto readable(const std::string& path) -> bool
{
  std::ifstream f(path);
  return static_cast<bool>(f);
}

}  // namespace

auto find_runfile(const std::string& logical, const std::string& argv0)
  -> std::string
{
  // Preferred: the manifest, which is authoritative and is the only mechanism
  // available on Windows.
  std::vector<std::string> manifests;
  if (const char* m = std::getenv("RUNFILES_MANIFEST_FILE"))
    manifests.emplace_back(m);
  for (const char* var : {"TEST_SRCDIR", "RUNFILES_DIR"})
    if (const char* root = std::getenv(var))
      manifests.push_back(std::string(root) + "/MANIFEST");
  // `bazel run` leaves the manifest beside the binary instead.
  if (!argv0.empty()) {
    manifests.push_back(argv0 + ".runfiles/MANIFEST");
    manifests.push_back(argv0 + ".runfiles_manifest");
    manifests.push_back(argv0 + ".exe.runfiles/MANIFEST");
    manifests.push_back(argv0 + ".exe.runfiles_manifest");
  }

  for (const std::string& manifest : manifests) {
    std::ifstream in(manifest);
    if (!in)
      continue;
    std::string line;
    while (std::getline(in, line)) {
      const size_t sep = line.find(' ');
      if (sep == std::string::npos || line.compare(0, sep, logical) != 0)
        continue;
      std::string real = line.substr(sep + 1);
      while (!real.empty() && (real.back() == '\r' || real.back() == '\n'))
        real.pop_back();
      if (readable(real))
        return real;
    }
  }

  // Materialised runfiles tree (Linux/macOS), or a run from the workspace root.
  std::vector<std::string> candidates;
  for (const char* var : {"TEST_SRCDIR", "RUNFILES_DIR"})
    if (const char* root = std::getenv(var))
      candidates.push_back(std::string(root) + "/" + logical);
  if (!argv0.empty())
    candidates.push_back(argv0 + ".runfiles/" + logical);
  candidates.push_back(logical);
  // Same path with the leading repository component removed.
  if (const size_t slash = logical.find('/'); slash != std::string::npos)
    candidates.push_back(logical.substr(slash + 1));

  for (const std::string& p : candidates)
    if (readable(p))
      return p;
  return {};
}

auto parse_json(const std::string& text, JsonValue& out, std::string& error)
  -> bool
{
  Parser p(text);
  if (!p.parse_value(out)) {
    error = p.error;
    return false;
  }
  // A recording line is exactly one JSON value; anything after it (past
  // whitespace) means the line is malformed, not a value we should accept.
  p.skip_ws();
  if (p.i != text.size()) {
    p.fail("trailing characters after JSON value");
    error = p.error;
    return false;
  }
  return true;
}

auto load_recording(const std::string& path, Recording& out, std::string& error)
  -> bool
{
  std::ifstream in(path);
  if (!in) {
    error = "cannot open " + path;
    return false;
  }

  std::set<int> boards;
  std::string line;
  size_t line_no = 0;

  while (std::getline(in, line)) {
    ++line_no;
    // Tolerate CRLF recordings on POSIX.
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
      line.pop_back();
    if (line.empty())
      continue;

    JsonValue rec;
    std::string parse_error;
    if (!parse_json(line, rec, parse_error)) {
      std::fprintf(stderr, "%s:%zu: skipping unparseable line (%s)\n",
                   path.c_str(), line_no, parse_error.c_str());
      continue;
    }
    if (rec.kind != JsonValue::Kind::Object)
      continue;

    const std::string kind = rec.string_or("t", "");

    if (kind == "meta") {
      out.created     = rec.string_or("created", "?");
      out.host        = rec.string_or("host", "?");
      out.platform    = rec.string_or("platform", "?");
      out.dds_version = rec.string_or("dds_version", "?");
      out.dds_mode    = rec.int_or("dds_mode", 1);
      out.threads     = rec.int_or("threads", 0);
      continue;
    }
    if (kind == "board")
      continue;  // a marker; the calls carry their own board number

    Call call;
    call.seq         = rec.int_or("seq", 0);
    call.board       = rec.int_or("board", 0);
    call.recorded_ms = rec.double_or("ms", 0.0);

    if (kind == "solve") {
      call.kind      = Call::Kind::Solve;
      call.purpose   = rec.string_or("purpose", "");
      call.trick     = rec.int_or("trick", 0);
      call.strain_i  = rec.int_or("strain_i", 0);
      call.leader_i  = rec.int_or("leader_i", 0);
      call.solutions = rec.int_or("solutions", 1);
      call.current_trick = to_int_vector(rec.find("current_trick"));

      const JsonValue* hands = rec.find("hands_pbn");
      if (hands == nullptr || hands->kind != JsonValue::Kind::Array ||
          hands->items.empty())
        continue;  // nothing to solve
      for (const auto& h : hands->items)
        if (h.kind == JsonValue::Kind::String)
          call.hands_pbn.push_back(h.text);

      call.result = to_result_map(rec.find("result"));
    } else if (kind == "par") {
      call.kind    = Call::Kind::Par;
      call.purpose = "par";
      call.trick   = 0;
      call.hand    = rec.string_or("hand", "");
      const JsonValue* v = rec.find("vuln");
      if (v != nullptr && v->kind == JsonValue::Kind::Array)
        for (const auto& b : v->items)
          call.vuln.push_back(b.kind == JsonValue::Kind::Bool ? b.boolean
                                                             : b.number != 0);
      const JsonValue* r = rec.find("result");
      call.par_result = (r != nullptr && r->kind == JsonValue::Kind::Number)
        ? static_cast<int>(r->number) : 0;
      if (call.hand.empty())
        continue;
    } else {
      continue;  // unknown record type
    }

    boards.insert(call.board);
    out.calls.push_back(std::move(call));
  }

  out.deals = static_cast<int>(boards.size());
  return true;
}

}  // namespace dds_replay
