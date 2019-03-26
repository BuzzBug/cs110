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

/*
TODO: 
declare the maps inside main and pass these maps by reference
ex) 
void foo()
{
  int x = 5;
  bar(x);
}
void bar(int &x){
  use x directly;
}

method for catching zombie child do not duplicate while(true) loops

*/  
std::map<int, std::string> systemCallNumbers;
std::map<std::string, int> systemCallNames;
std::map<std::string, systemCallSignature> systemCallSignatures;
std::map<int, std::string> errorConstants;

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
    raise(SIGSTOP);
    int start = 1 + numFlags;
    execvp(argv[start], argv + start);
  }
  
  waitpid(pid, NULL, 0); 
  ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
  while(true)
  {
    int status;
    long returnvalue;
    string command;
    while(true)
    {
      ptrace(PTRACE_SYSCALL, pid, 0, 0);
      waitpid(pid, &status, 0);
      if (WIFSTOPPED(status) && (WSTOPSIG(status) & 0x80))
      {
          int status;
          long systemcallno, returnvalue;
          systemcallno = ptrace(PTRACE_PEEKUSER, pid, ORIG_RAX * sizeof(long));

          if (simple)
          {
            cout << "syscall(" << systemcallno << ")" << endl;
            cout.flush();
            break;  
          }
          else
          {
            command = systemCallNumbers.find(systemcallno)->second;
          }

          command = systemCallNumbers.find(systemcallno)->second;
          cout << endl << command << endl;
          vector<scParamType> vec;
          int registers[] = {RDI, RSI, RDX, R10, R8, R9};
          auto v = systemCallSignatures.find(command);
          if (v  != systemCallSignatures.end())
          {
            vec = systemCallSignatures.find(command)->second;
            int index = 0;
            for (auto elem : vec)
            {
              
              if (elem == SYSCALL_STRING)
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
                        outlen = i + 1;
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
                cout << "syscall_str: " << str << endl;
                cout.flush();
              }
              
              
              if (elem == SYSCALL_POINTER)
              {
                long outlong;
                outlong  = ptrace(PTRACE_PEEKUSER, pid, registers[index] * sizeof(long));
                cout <<  "syscall_pointer: " << (void *)outlong << endl;
                cout.flush();
              }
              
              if (elem == SYSCALL_INTEGER)
              {
                long outlong;
                outlong = ptrace(PTRACE_PEEKUSER, pid, registers[index] * sizeof(long));
                cout << "long of syscall int: " << outlong << endl;
                cout << "syscall_integer: " << (int)outlong << endl;
                cout.flush();
              }
              index++;
            }  
          }
          break;
      }

      if (WIFEXITED(status))
      {
        printf("Program exited normally\n");
        fflush(stdout);
        exit(0);
      }
    }
    
    while(true)
    {
      int status2;
      ptrace(PTRACE_SYSCALL, pid, 0, 0);
      waitpid(pid, &status2, 0);
      if (WIFSTOPPED(status2) && (WSTOPSIG(status2) & 0x80))
      {
          std::string str1 ("brk");
          std::string str2 ("sbrk");
          std::string str3 ("mmap");
          returnvalue = ptrace(PTRACE_PEEKUSER, pid, RAX * sizeof(long));  
          if ((command.compare(str1) == 0) || (command.compare(str2) == 0) || (command.compare(str3) == 0))
          {
            void *outvp = reinterpret_cast<void *>(returnvalue);
            cout << "return value as ptr: " << outvp << endl;
          }
          else
          {
            cout << "return value: " << returnvalue << endl;
          }
          if (returnvalue < 0)
          {
            cout << "errno: " << errorConstants[abs(returnvalue)] << endl;
          }
          cout.flush();
          break;
      }
      if (WIFEXITED(status2))
      {
        printf("<no return>\n");
        printf("Program exited normally.\n");
        fflush(stdout);
        exit(0);
      }
    }
  }
}
