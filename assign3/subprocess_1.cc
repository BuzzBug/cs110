/**
 * File: subprocess.cc
 * -------------------
 * Presents the implementation of the subprocess routine.
 */

#include "subprocess.h"
using namespace std;

subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) throw (SubprocessException) {
  int supplyfds[2];
  int ingestfds[2];
 
  int supplyfd = kNotInUse;
  int ingestfd = kNotInUse;
  if (supplyChildInput)
  {
    pipe(supplyfds);
    //supplyfd: fd you write to would be the write end of the supply pipe
    //child will read from the read end of the supply pipe
    supplyfd = supplyfds[1];
  }
  if(ingestChildOutput)
  {
    pipe(ingestfds);
    //ingestfd: fd you read from would be the read end of the ingest pipe
    //child would write to the write end of the ingest pipe
    ingestfd = ingestfds[0];
  }
  subprocess_t process = {fork(), supplyfd, ingestfd};
  if (process.pid == 0)
  {
    //enable: parent process to pipe content to the new process's stdin
    if(supplyChildInput)
    {
      //associate STDIN of child process with the read end of the supply
      //pipe
      dup2(supplyfds[0], STDIN_FILENO);
      close(supplyfds[0]);
      //child will not need to write anything to supplyfds
      close(supplyfds[1]);
    }
    else
    {
      close(supplyfds[0]);
      close(supplyfds[1]);
    }
    //enable: parent process to read content from child's stdout
    if(ingestChildOutput)
    {
      //associate STDOUT of child process with the write end of the ingest
      //pipe
      dup2(ingestfds[1], STDOUT_FILENO);
      close(ingestfds[1]);
      //child will not need to read anything from ingestfds
      close(ingestfds[0]);
    }
    else
    {
      close(ingestfds[0]);
      close(ingestfds[1]);
    }
    execvp(argv[0], argv);
  }
  close(supplyfds[0]);
  close(ingestfds[1]);
  return process;
}
