/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "bridge_solver_runner.hpp"

#include <array>
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

bool parse_pbn_hands(
  const std::string& pbn,
  std::array<std::array<std::string, DDS_SUITS>, DDS_HANDS>& hands,
  std::string* error)
{
  const std::string trimmed = trim_copy(pbn);
  if (trimmed.size() < 2 ||
    (trimmed[0] != 'N' && trimmed[0] != 'n') ||
    trimmed[1] != ':')
  {
    set_error(error, "PBN must start with N:");
    return false;
  }

  const std::vector<std::string> seats = split_ws(trimmed.substr(2));
  if (seats.size() != static_cast<size_t>(DDS_HANDS))
  {
    set_error(error, "PBN must list four seat holdings");
    return false;
  }

  for (int seat = 0; seat < DDS_HANDS; ++seat)
  {
    std::array<std::string, DDS_SUITS> suits{};
    std::string cur;
    int suit = 0;
    for (char ch : seats[static_cast<size_t>(seat)])
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
    hands[static_cast<size_t>(seat)] = suits;
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

  std::string cmd =
    quote(binary) + " -i -f " + quote(deal_path) + " -m0";
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
  char* argv[] = {
    binary_mut.data(),
    arg_i,
    arg_f,
    deal_mut.data(),
    arg_m0,
    nullptr};

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

}  // namespace

std::string pbn_to_macroxue_deal(
  const std::string& pbn,
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
       << hand_line(2, 14) << '\n'   // S
       << '\n';
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
    binary, temp_path.string(), stdout_text, error);
  std::error_code ec;
  fs::remove(temp_path, ec);
  if (!ran)
    return false;

  return parse_macroxue_solver_stdout(stdout_text, out, error);
}
