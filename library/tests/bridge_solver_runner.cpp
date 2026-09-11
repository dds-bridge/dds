/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "bridge_solver_runner.hpp"

#include <array>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#if defined(__EMSCRIPTEN__)
// External process spawn is not supported.
#elif defined(_WIN32)
#include <windows.h>
#else
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace
{

constexpr int kSnweToNesw[DDS_HANDS] = {2, 0, 3, 1};  // S N W E → N E S W

void set_error(std::string* error, const std::string& message)
{
  if (error != nullptr)
    *error = message;
}

std::string trim_copy(std::string s)
{
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
    s.erase(s.begin());
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    s.pop_back();
  return s;
}

std::vector<std::string> split_ws(const std::string& line)
{
  std::vector<std::string> tokens;
  std::istringstream in(line);
  std::string tok;
  while (in >> tok)
    tokens.push_back(tok);
  return tokens;
}

std::string suit_token(const std::string& holding)
{
  return holding.empty() ? "-" : holding;
}

int seat_letter_to_dds(char letter)
{
  switch (static_cast<char>(std::toupper(static_cast<unsigned char>(letter))))
  {
    case 'N': return 0;
    case 'E': return 1;
    case 'S': return 2;
    case 'W': return 3;
    default: return -1;
  }
}

bool parse_pbn_hands(
  const std::string& pbn,
  std::array<std::array<std::string, DDS_SUITS>, DDS_HANDS>& hands,
  std::string* error)
{
  const std::string trimmed = trim_copy(pbn);
  if (trimmed.size() < 2 || trimmed[1] != ':')
  {
    set_error(error, "PBN must start with a seat letter and ':'");
    return false;
  }

  const int start_seat = seat_letter_to_dds(trimmed[0]);
  if (start_seat < 0)
  {
    set_error(error, "PBN must start with N:, E:, S:, or W:");
    return false;
  }

  const std::vector<std::string> seats = split_ws(trimmed.substr(2));
  if (seats.size() != static_cast<size_t>(DDS_HANDS))
  {
    set_error(error, "PBN must list four seat holdings");
    return false;
  }

  for (int i = 0; i < DDS_HANDS; ++i)
  {
    std::array<std::string, DDS_SUITS> suits{};
    std::string cur;
    int suit = 0;
    for (char ch : seats[static_cast<size_t>(i)])
    {
      if (ch == '.')
      {
        if (suit >= DDS_SUITS)
        {
          set_error(error, "PBN hand has too many suit separators");
          return false;
        }
        suits[static_cast<size_t>(suit++)] = cur;
        cur.clear();
        continue;
      }
      cur.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }
    if (suit != DDS_SUITS - 1)
    {
      set_error(error, "PBN hand must have four suits separated by '.'");
      return false;
    }
    suits[static_cast<size_t>(suit)] = cur;
    // Hands are listed clockwise from the opening seat letter.
    const int dds_seat = (start_seat + i) % DDS_HANDS;
    hands[static_cast<size_t>(dds_seat)] = suits;
  }
  return true;
}

int strain_letter_to_dds(char letter)
{
  switch (static_cast<char>(std::toupper(static_cast<unsigned char>(letter))))
  {
    case 'S': return 0;
    case 'H': return 1;
    case 'D': return 2;
    case 'C': return 3;
    case 'N': return 4;
    default: return -1;
  }
}

#if defined(__EMSCRIPTEN__)

bool run_solver_capture_stdout(
  const std::string& /*binary*/,
  const std::string& /*deal_path*/,
  bool /*ignore_trump_and_lead*/,
  std::string& /*stdout_text*/,
  std::string* error)
{
  set_error(error, "bridge-solver backend is not supported under emscripten");
  return false;
}

#elif defined(_WIN32)

bool run_solver_capture_stdout(
  const std::string& binary,
  const std::string& deal_path,
  bool ignore_trump_and_lead,
  std::string& stdout_text,
  std::string* error)
{
  auto quote = [](const std::string& s) {
    std::string out = "\"";
    for (char ch : s)
    {
      if (ch == '"')
        out += "\\\"";
      else
        out.push_back(ch);
    }
    out.push_back('"');
    return out;
  };

  std::string cmd = quote(binary);
  if (ignore_trump_and_lead)
    cmd += " -i";
  cmd += " -f " + quote(deal_path) + " -m0";
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  HANDLE read_pipe = nullptr;
  HANDLE write_pipe = nullptr;
  if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0))
  {
    set_error(error, "CreatePipe failed");
    return false;
  }
  SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = write_pipe;
  si.hStdError = write_pipe;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  PROCESS_INFORMATION pi{};

  std::vector<char> cmdline(cmd.begin(), cmd.end());
  cmdline.push_back('\0');
  if (!CreateProcessA(
      binary.c_str(),
      cmdline.data(),
      nullptr,
      nullptr,
      TRUE,
      0,
      nullptr,
      nullptr,
      &si,
      &pi))
  {
    CloseHandle(read_pipe);
    CloseHandle(write_pipe);
    set_error(error, "CreateProcess failed for bridge-solver");
    return false;
  }
  CloseHandle(write_pipe);

  char buffer[4096];
  DWORD nread = 0;
  stdout_text.clear();
  while (ReadFile(read_pipe, buffer, sizeof(buffer), &nread, nullptr) && nread > 0)
    stdout_text.append(buffer, buffer + nread);
  CloseHandle(read_pipe);

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 1;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  if (exit_code != 0)
  {
    set_error(
      error,
      "bridge-solver failed (exit " + std::to_string(exit_code) + ")");
    return false;
  }
  return true;
}

#else

bool run_solver_capture_stdout(
  const std::string& binary,
  const std::string& deal_path,
  bool ignore_trump_and_lead,
  std::string& stdout_text,
  std::string* error)
{
  // Use posix_spawn rather than popen/fork: DDS may already have threads, and
  // fork-after-threads makes the /bin/sh child fail with wait status 32512
  // (exit 127 / command not found).
  int pipefd[2];
  if (pipe(pipefd) != 0)
  {
    set_error(error, "pipe failed for bridge-solver");
    return false;
  }

  posix_spawn_file_actions_t actions;
  if (posix_spawn_file_actions_init(&actions) != 0)
  {
    close(pipefd[0]);
    close(pipefd[1]);
    set_error(error, "posix_spawn_file_actions_init failed");
    return false;
  }

  const int fa1 = posix_spawn_file_actions_adddup2(
    &actions, pipefd[1], STDOUT_FILENO);
  const int fa2 = posix_spawn_file_actions_adddup2(
    &actions, pipefd[1], STDERR_FILENO);
  const int fa3 = posix_spawn_file_actions_addclose(&actions, pipefd[0]);
  const int fa4 = posix_spawn_file_actions_addclose(&actions, pipefd[1]);
  if (fa1 != 0 || fa2 != 0 || fa3 != 0 || fa4 != 0)
  {
    posix_spawn_file_actions_destroy(&actions);
    close(pipefd[0]);
    close(pipefd[1]);
    set_error(error, "posix_spawn file actions failed");
    return false;
  }

  std::vector<char> binary_mut(binary.begin(), binary.end());
  binary_mut.push_back('\0');
  std::vector<char> deal_mut(deal_path.begin(), deal_path.end());
  deal_mut.push_back('\0');
  char arg_i[] = "-i";
  char arg_f[] = "-f";
  char arg_m0[] = "-m0";
  char* argv_ignore[] = {
    binary_mut.data(),
    arg_i,
    arg_f,
    deal_mut.data(),
    arg_m0,
    nullptr};
  char* argv_respect[] = {
    binary_mut.data(),
    arg_f,
    deal_mut.data(),
    arg_m0,
    nullptr};
  char** argv = ignore_trump_and_lead ? argv_ignore : argv_respect;

  pid_t pid = 0;
  const int spawn_rc = posix_spawn(
    &pid, binary.c_str(), &actions, nullptr, argv, environ);
  posix_spawn_file_actions_destroy(&actions);
  close(pipefd[1]);

  if (spawn_rc != 0)
  {
    close(pipefd[0]);
    set_error(
      error,
      std::string("posix_spawn failed: ") + std::strerror(spawn_rc));
    return false;
  }

  stdout_text.clear();
  char buffer[4096];
  ssize_t nread = 0;
  while ((nread = read(pipefd[0], buffer, sizeof(buffer))) > 0)
    stdout_text.append(buffer, static_cast<size_t>(nread));
  close(pipefd[0]);

  int status = 0;
  if (waitpid(pid, &status, 0) < 0)
  {
    set_error(error, "waitpid failed for bridge-solver");
    return false;
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
  {
    const int code = WIFEXITED(status) ? WEXITSTATUS(status) : status;
    set_error(
      error,
      "bridge-solver failed (exit " + std::to_string(code) + ")");
    return false;
  }
  return true;
}

#endif

char strain_to_letter(int trump)
{
  static constexpr char kLetters[] = {'S', 'H', 'D', 'C', 'N'};
  if (trump < 0 || trump >= DDS_STRAINS)
    return '?';
  return kLetters[trump];
}

char seat_to_letter(int first)
{
  static constexpr char kLetters[] = {'N', 'E', 'S', 'W'};
  if (first < 0 || first >= DDS_HANDS)
    return '?';
  return kLetters[first];
}

int count_cards_in_pbn_hand0(const std::string& pbn)
{
  std::array<std::array<std::string, DDS_SUITS>, DDS_HANDS> hands{};
  if (!parse_pbn_hands(pbn, hands, nullptr))
    return -1;
  int n = 0;
  for (int suit = 0; suit < DDS_SUITS; ++suit)
    n += static_cast<int>(hands[0][static_cast<size_t>(suit)].size());
  return n;
}

}  // namespace

std::string pbn_to_macroxue_deal(
  const std::string& pbn,
  std::string* error)
{
  return pbn_to_macroxue_deal(pbn, /*trump=*/-1, /*first=*/-1, error);
}

std::string pbn_to_macroxue_deal(
  const std::string& pbn,
  const int trump,
  const int first,
  std::string* error)
{
  std::array<std::array<std::string, DDS_SUITS>, DDS_HANDS> hands{};
  if (!parse_pbn_hands(pbn, hands, error))
    return {};

  auto hand_line = [&](int seat, int indent) -> std::string {
    std::ostringstream out;
    out << std::string(static_cast<size_t>(indent), ' ');
    for (int suit = 0; suit < DDS_SUITS; ++suit)
    {
      if (suit > 0)
        out << ' ';
      out << suit_token(hands[static_cast<size_t>(seat)][static_cast<size_t>(suit)]);
    }
    return out.str();
  };

  std::ostringstream west_east;
  for (int suit = 0; suit < DDS_SUITS; ++suit)
  {
    if (suit > 0)
      west_east << ' ';
    west_east << suit_token(hands[3][static_cast<size_t>(suit)]);  // W
  }
  west_east << "           ";
  for (int suit = 0; suit < DDS_SUITS; ++suit)
  {
    if (suit > 0)
      west_east << ' ';
    west_east << suit_token(hands[1][static_cast<size_t>(suit)]);  // E
  }

  std::ostringstream body;
  body << hand_line(0, 14) << '\n'   // N
       << west_east.str() << '\n'
       << hand_line(2, 14) << '\n';  // S
  if (trump >= 0 && trump < DDS_STRAINS && first >= 0 && first < DDS_HANDS)
  {
    body << strain_to_letter(trump) << '\n'
         << seat_to_letter(first) << '\n';
  }
  else
  {
    body << '\n';
  }
  return body.str();
}

bool parse_macroxue_solver_stdout(
  const std::string& text,
  DdTableResults& out,
  std::string* error)
{
  bool found[DDS_STRAINS] = {false, false, false, false, false};
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line))
  {
    const std::string trimmed = trim_copy(line);
    if (trimmed.empty())
      continue;
    const std::vector<std::string> tokens = split_ws(trimmed);
    if (tokens.size() < 5)
      continue;
    if (tokens[0].size() != 1)
      continue;
    const int strain = strain_letter_to_dds(tokens[0][0]);
    if (strain < 0)
      continue;

    int snwe[DDS_HANDS] = {};
    bool ok = true;
    for (int i = 0; i < DDS_HANDS; ++i)
    {
      try
      {
        size_t idx = 0;
        snwe[i] = std::stoi(tokens[static_cast<size_t>(i + 1)], &idx);
        if (idx != tokens[static_cast<size_t>(i + 1)].size())
          ok = false;
      }
      catch (...)
      {
        ok = false;
      }
      if (!ok)
        break;
    }
    if (!ok)
      continue;

    for (int src = 0; src < DDS_HANDS; ++src)
      out.res_table[strain][kSnweToNesw[src]] = snwe[src];
    found[strain] = true;
  }

  std::string missing;
  constexpr char kLetters[] = {'S', 'H', 'D', 'C', 'N'};
  for (int strain = 0; strain < DDS_STRAINS; ++strain)
  {
    if (found[strain])
      continue;
    if (!missing.empty())
      missing.push_back(',');
    missing.push_back(kLetters[strain]);
  }
  if (!missing.empty())
  {
    set_error(error, "macroxue output missing strain line(s): " + missing);
    return false;
  }
  return true;
}

bool parse_macroxue_solver_solve_stdout(
  const std::string& text,
  int& tricks,
  std::string* error)
{
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line))
  {
    const std::string trimmed = trim_copy(line);
    if (trimmed.empty())
      continue;
    const std::vector<std::string> tokens = split_ws(trimmed);
    // Fixed-lead form: "<strain> <tricks> <time> s …" (not four SNWE counts).
    if (tokens.size() < 3 || tokens[0].size() != 1)
      continue;
    if (strain_letter_to_dds(tokens[0][0]) < 0)
      continue;
    // Reject full-table lines where tokens[2] is another integer trick count.
    const bool second_is_int = !tokens[2].empty() &&
      std::all_of(tokens[2].begin(), tokens[2].end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
      });
    if (second_is_int)
      continue;
    try
    {
      size_t idx = 0;
      tricks = std::stoi(tokens[1], &idx);
      if (idx == tokens[1].size())
        return true;
    }
    catch (...)
    {
    }
  }
  set_error(error, "macroxue solve output missing trick count");
  return false;
}

auto leading_side_tricks_from_declarer_side(
  const int remaining_tricks,
  const int declarer_side_tricks) -> int
{
  return remaining_tricks - declarer_side_tricks;
}

bool run_bridge_solver_table(
  const std::string& binary,
  const std::string& pbn,
  DdTableResults& out,
  std::string* error)
{
  const std::string deal = pbn_to_macroxue_deal(pbn, error);
  if (deal.empty())
    return false;

  namespace fs = std::filesystem;
  const fs::path temp_path =
    fs::temp_directory_path() /
    ("dds_bridgesolver_" + std::to_string(
      static_cast<unsigned long long>(
        reinterpret_cast<uintptr_t>(&out))) + ".deal");

  {
    std::ofstream file(temp_path);
    if (!file)
    {
      set_error(error, "failed to write temp deal file");
      return false;
    }
    file << deal;
  }

  std::string stdout_text;
  const bool ran = run_solver_capture_stdout(
    binary, temp_path.string(), /*ignore_trump_and_lead=*/true, stdout_text, error);
  std::error_code ec;
  fs::remove(temp_path, ec);
  if (!ran)
    return false;

  return parse_macroxue_solver_stdout(stdout_text, out, error);
}

bool run_bridge_solver_solve(
  const std::string& binary,
  const std::string& pbn,
  const int trump,
  const int first,
  int& leading_side_tricks,
  std::string* error)
{
  if (trump < 0 || trump >= DDS_STRAINS || first < 0 || first >= DDS_HANDS)
  {
    set_error(error, "trump/first out of range for bridge-solver solve");
    return false;
  }

  const int remaining = count_cards_in_pbn_hand0(pbn);
  if (remaining < 0)
  {
    set_error(error, "failed to count remaining cards in PBN");
    return false;
  }

  const std::string deal = pbn_to_macroxue_deal(pbn, trump, first, error);
  if (deal.empty())
    return false;

  namespace fs = std::filesystem;
  const fs::path temp_path =
    fs::temp_directory_path() /
    ("dds_bridgesolver_solve_" + std::to_string(
      static_cast<unsigned long long>(
        reinterpret_cast<uintptr_t>(&leading_side_tricks))) + ".deal");

  {
    std::ofstream file(temp_path);
    if (!file)
    {
      set_error(error, "failed to write temp deal file");
      return false;
    }
    file << deal;
  }

  std::string stdout_text;
  const bool ran = run_solver_capture_stdout(
    binary,
    temp_path.string(),
    /*ignore_trump_and_lead=*/false,
    stdout_text,
    error);
  std::error_code ec;
  fs::remove(temp_path, ec);
  if (!ran)
    return false;

  int declarer_side = 0;
  if (!parse_macroxue_solver_solve_stdout(stdout_text, declarer_side, error))
    return false;

  leading_side_tricks =
    leading_side_tricks_from_declarer_side(remaining, declarer_side);
  return true;
}
