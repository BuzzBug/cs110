/**
 * File: trace.cc
 * ----------------
 * Presents the implementation of the trace program, which traces the execution of another
 * program and prints out information about ever single system call it makes.  For each system call,
 * trace prints:
 *
 *    + the name of the system call,
 *    + the values of all of its arguments, and
 *    + the system calls return value
 */
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <unistd.h> // for fork, execvp
#include <string.h> // for memchr, strerror
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include "trace-options.h"
#include "trace-error-constants.h"
#include "trace-system-calls.h"
#include "trace-exception.h"
using namespace std;

map<int, string> systemCallNumbers;
map<string, int> systemCallNames;
map<string, systemCallSignature> systemCallSignatures;
map<int, string> errorConstants;

int main(int argc, char *argv[]) {
  bool simple = false, rebuild = false;
  int numFlags = processCommandLineFlags(simple, rebuild, argv);
  if (argc - numFlags == 1) {
    cout << "Nothing to trace... exiting." << endl;
    return 0;
  }

  compileSystemCallData(systemCallNumbers, systemCallNames, systemCallSignatures, rebuild);
  compileSystemCallErrorStrings(errorConstants); 
  
  pid_t pid = fork();

  if (pid == 0)
  {
    ptrace(PTRACE_TRACEME);
    raise(SIGSTOP); //stop the child from launching until waitstatus is set
    int start = 1 + numFlags;
    execvp(argv[start], argv + start);
  }

  waitpid(pid, NULL, 0); //suspend the parent until the child has finished attachment process 
  ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD); //set the waitstatus, child can execvp now
  while(true)
  {
    while(true)
    {
      int status;
      ptrace(PTRACE_SYSCALL, pid, 0, 0);
      waitpid(pid, &status, 0);
      if (WIFSTOPPED(status) && (WSTOPSIG(status) & 0x80))
      {
        long systemcallno;
        string command;
        systemcallno = ptrace(PTRACE_PEEKUSER, pid, ORIG_RAX * sizeof(long));
        if (simple) 
        {
          cout << "syscall(" << systemcallno << ")" << endl;
          cout.flush();
          break;
        }
        HandleFull(systemcallno);
      }  
    }


  }
  return 0;
}
