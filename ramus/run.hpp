#pragma once

//auto looseExecute(const string& name, const string& args...) -> string;
//[[synchronous]]
//executes program and waits for completion without handle inheritance, which
//makes it impossible to read stdout data

#include <nall/intrinsics.hpp>
#include <nall/string.hpp>

namespace ramus {

#if defined(PLATFORM_MACOSX) || defined(PLATFORM_LINUX) || defined(PLATFORM_BSD)

template<typename... P> inline auto looseExecute(const nall::string& name, P&&... p) -> nall::execute_result_t {
  int fdout[2];
  int fderr[2];
  if(pipe(fdout) == -1) return {};
  if(pipe(fderr) == -1) return {};

  pid_t pid = fork();
  if(pid == 0) {
    const char* argv[1 + sizeof...(p) + 1];
    const char** argp = argv;
    nall::string_vector argl(forward<P>(p)...);
    *argp++ = (const char*)name;
    for(auto& arg : argl) *argp++ = (const char*)arg;
    *argp++ = nullptr;

    dup2(fdout[1], STDOUT_FILENO);
    dup2(fderr[1], STDERR_FILENO);
    close(fdout[0]);
    close(fderr[0]);
    close(fdout[1]);
    close(fderr[1]);
    execvp(name, (char* const*)argv);
    //this is called only if execvp fails:
    //use _exit instead of exit, to avoid destroying key shared file descriptors
    _exit(EXIT_FAILURE);
  } else {
    close(fdout[1]);
    close(fderr[1]);

    char buffer[256];
    nall::execute_result_t result;

    close(fdout[0]);
    close(fderr[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    if(!WIFEXITED(status)) return {};
    result.code = WEXITSTATUS(status);
    return result;
  }
}

#elif defined(PLATFORM_WINDOWS)

template<typename... P> inline auto looseExecute(const nall::string& name, P&&... p) -> nall::execute_result_t {
  nall::string_vector argl(name, forward<P>(p)...);
  for(auto& arg : argl) if(arg.find(" ")) arg = {"\"", arg, "\""};
  nall::string arguments = argl.merge(" ");

  SECURITY_ATTRIBUTES sa;
  ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = true;
  sa.lpSecurityDescriptor = nullptr;

  HANDLE stdoutRead;
  HANDLE stdoutWrite;
  if(!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0)) return {};
  if(!SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0)) return {};

  HANDLE stderrRead;
  HANDLE stderrWrite;
  if(!CreatePipe(&stderrRead, &stderrWrite, &sa, 0)) return {};
  if(!SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0)) return {};

  HANDLE stdinRead;
  HANDLE stdinWrite;
  if(!CreatePipe(&stdinRead, &stdinWrite, &sa, 0)) return {};
  if(!SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0)) return {};

  STARTUPINFO si;
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.hStdOutput = stdoutWrite;
  si.hStdError = stderrWrite;
  si.hStdInput = stdinRead;
  si.dwFlags = STARTF_USESTDHANDLES;

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  if(!CreateProcess(
    nullptr, utf16_t(arguments),
    nullptr, nullptr, false, CREATE_NO_WINDOW,
    nullptr, nullptr, &si, &pi
  )) return {};

  DWORD exitCode = EXIT_FAILURE;
  if(WaitForSingleObject(pi.hProcess, INFINITE)) return {};
  if(!GetExitCodeProcess(pi.hProcess, &exitCode)) return {};
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  char buffer[256];
  nall::execute_result_t result;
  result.code = exitCode;

  return result;
}

#else

template<typename... P> inline auto looseExecute(const nall::string& name, P&&... p) -> nall::string {
  return "";
}

#endif

}
