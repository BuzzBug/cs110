/**
 * File: subprocess.cc
 * -------------------
 * Presents the implementation of the subprocess routine.
 */

#include "subprocess.h"
#include "exit-utils.h"

using namespace std;

static const int kExecFailed = 1;

static void dup2wrapper(int fd1, int fd2)
{
  int err = dup2(fd1, fd2);
  if (err == -1) throw(SubprocessException("Dup2 failed.\n"));
  return;
}

static void closefd(int fd)
{
  int err = close(fd);
  if (err == -1) throw(SubprocessException("Close failed.\n"));
  return;
}

static void pipewrapper(int fds[])
{
  int err = pipe(fds);
  if (err == -1) throw(SubprocessException("Pipe failed.\n"));
  return;
}

subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) throw (SubprocessException) {
  int supplyfds[2] = {kNotInUse};
  int ingestfds[2] = {kNotInUse};
 
  int supplyfd = kNotInUse;
  int ingestfd = kNotInUse;
  
  if (supplyChildInput)
  {
    pipewrapper(supplyfds);
    //supplyfd: fd you write to would be the write end of the supply pipe
    //child will read from the read end of the supply pipe
    supplyfd = supplyfds[1];
  }
  if(ingestChildOutput)
  {
    pipewrapper(ingestfds);
    //ingestfd: fd you read from would be the read end of the ingest pipe
    //child would write to the write end of the ingest pipe
    ingestfd = ingestfds[0];
  }
  
  pid_t pid = fork();
  if (pid == -1) throw(SubprocessException("Fork failed.\n"));
  
  subprocess_t process = {pid, supplyfd, ingestfd};
  if (process.pid == 0)
  {
    //enable: parent process to pipe content to the new process's stdin
    if(supplyChildInput)
    {
       //child will not need to write anything to supplyfds
      closefd(supplyfds[1]);
      //associate STDIN of child process with the read end of the supply
      //pipe
      dup2wrapper(supplyfds[0], STDIN_FILENO);
      closefd(supplyfds[0]);
    }
    //enable: parent process to read content from child's stdout
    if(ingestChildOutput)
    {
       //child will not need to read anything from ingestfds
      closefd(ingestfds[0]);

      //associate STDOUT of child process with the write end of the ingest
      //pipe
      dup2wrapper(ingestfds[1], STDOUT_FILENO);
      closefd(ingestfds[1]);
    }
    execvp(argv[0], argv);
    exitIf(true, kExecFailed, stderr, "execvp failed.");
  }
  if (supplyChildInput) 
  {
    closefd(supplyfds[0]);
  }
  if (ingestChildOutput) 
  {
    closefd(ingestfds[1]);
  }
  return process;
}
