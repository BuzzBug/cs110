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
int registers[] = {RDI, RSI, RDX, R10, R8, R9};
set<string> specialcommands {"brk", "sbrk", "mmap"};

static string handleString(pid_t pid, int index)
{
  long baseaddr = ptrace(PTRACE_PEEKUSER, pid, registers[index] * sizeof(long));
  string str;
  size_t numBytesRead = 0;
  bool unseen = true;
  while(true)
  {
    long outlong = ptrace(PTRACE_PEEKDATA, pid, baseaddr + numBytesRead);
    void *outvp = reinterpret_cast<void *>(&outlong);
    char *out = (char *)outvp;
    size_t outlen = sizeof(long);
    for (int i = 0; i < 8; ++i)
    {
      if(out[i] == '\0')
      {
        outlen = i;
        unseen = false;
        break;
      }
    }
    str.append(out, outlen);
    numBytesRead += outlen;
    if (!unseen)
    {
      break;
    }
  }
  return str;
}

static void outputFull(long systemcallno, string command, pid_t pid)
{
  vector<scParamType> vec;
  auto v = systemCallSignatures.find(command);
  if (v != systemCallSignatures.end())
  {
    vec = v->second;
    int index = 0;
    int len = vec.size();
    cout << command << "(";
    for (auto &elem : vec)
    {
      if (elem == SYSCALL_STRING)
      {
        string outputstr = handleString(pid, index);
        cout << "\"" << outputstr << "\""; 
      }
      if (elem == SYSCALL_POINTER)
      {
        long outlong;
        outlong = ptrace(PTRACE_PEEKUSER, pid, registers[index] * sizeof(long));
        void *outptr = reinterpret_cast<void *>(outlong);
        if (outptr == 0)
        {
          cout << "NULL";
        }
        else
        {
          cout << outptr;
        }
      }
      if (elem == SYSCALL_INTEGER)
      {
        long outlong;
        outlong = ptrace(PTRACE_PEEKUSER, pid, registers[index] * sizeof(long));
        cout << (int)outlong;
      }
      if (index != (len - 1))
      {
        cout << ", ";
      }
      index++;
    }
    cout << ") = ";
    cout.flush();
  }
  else
  {
    cout << command << "(<signature-information-missing>) = ";
  }
}

static void outputSimple(long systemcallno)
{
  cout << "syscall(" << systemcallno << ") = ";
  cout.flush();
}

static void outputSimpleReturn(long returnvalue)
{
  cout << (int)returnvalue << endl;
}

static void outputFullReturn(long returnvalue, string command)
{
  if(specialcommands.find(command) != specialcommands.end())
  {
    void *outvp = reinterpret_cast<void *>(returnvalue);
    cout << outvp;
  }
  else
  {
    if (returnvalue < 0)
    {
      cout << "-1 " << errorConstants[abs(returnvalue)] << " (" << strerror(abs(returnvalue)) << ")";
    }
    else
    {
      //new: returnvalue
      cout << (int)returnvalue;
    }
  }
  cout << endl;
  cout.flush();
}

static void RunTrace(int numFlags, bool simple, bool rebuild, char *argv[])
{
  pid_t pid = fork();
  if (pid == 0)
  {
    ptrace(PTRACE_TRACEME);
    raise(SIGSTOP);
    execvp(argv[numFlags + 1], argv + numFlags + 1);
  }
  waitpid(pid, NULL, 0);
  ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
  while(true)
  {
    string command;
    while(true)
    {
      int status;
      ptrace(PTRACE_SYSCALL, pid, 0, 0);
      waitpid(pid, &status, 0);
      
      if (WIFSTOPPED(status) && (WSTOPSIG(status) & 0x80))
      {
        long systemcallno;
        systemcallno = ptrace(PTRACE_PEEKUSER, pid, ORIG_RAX * sizeof(long));
        command = systemCallNumbers.find(systemcallno)->second;
        if(simple)
        {
          outputSimple(systemcallno);
        }
        else
        {
          outputFull(systemcallno, command, pid);
        }
        break;
      }
    }

    while(true)
    {
      int status2;
      ptrace(PTRACE_SYSCALL, pid, 0, 0);
      waitpid(pid, &status2, 0);
      if (WIFSTOPPED(status2) && (WSTOPSIG(status2) & 0x80))
      {
        long returnvalue = ptrace(PTRACE_PEEKUSER, pid, RAX * sizeof(long));
        if (simple)
        {
          outputSimpleReturn(returnvalue);
        }
        else
        {
          outputFullReturn(returnvalue, command);
        }
        break;
      }
      if (WIFEXITED(status2))
      {
        cout << "<no return>" << endl << "Program exited normally with status " << WEXITSTATUS(status2);
        cout.flush();
        exit(0);
      }  
    }
  }
}
int main(int argc, char *argv[]) {
  bool simple = false, rebuild = false;
  int numFlags = processCommandLineFlags(simple, rebuild, argv);
  if (argc - numFlags == 1) {
    cout << "Nothing to trace... exiting." << endl;
    return 0;
  }

  compileSystemCallData(systemCallNumbers, systemCallNames, systemCallSignatures, rebuild);
  compileSystemCallErrorStrings(errorConstants); 
  RunTrace(numFlags, simple, rebuild, argv);
  return 0;
}
