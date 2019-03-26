/**
 * File: pipeline.c
 * ----------------
 * Presents the implementation of the pipeline routine.
 */
 
#include "pipeline.h"
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

void pipeline(char *argv1[], char *argv2[], pid_t pids[]) {
  int fds[2];
  pipe(fds);
  //need a pipe connecting the STDOUT of the first process to the STDIN of
  //the second process
  //write to fds[1], all output written to fds[1] can be read from fds[0]
  //dup2 STDIN of the second process to the read end of the pipe, so the
  //second process will be getting content from fds[0]
  //dup2 STDOUT of second process to the write end of the pipe, so the
  //first process will not be publishing  
  //
  pid_t process1pid = fork();
  pids[0] = process1pid;
  if (process1pid == 0)
  {
    //process 1 does not need to read anything
    close(fds[0]);
    //process 1 needs to publish to write end of the pipe
    //associate write end of the pipe with STDOUT of the second process.
    dup2(fds[1], STDOUT_FILENO);
    close(fds[1]);
    execvp(argv1[0], argv1);
  }
  close(fds[1]);
  pid_t process2pid = fork();
  pids[1] = process2pid;
  if (process2pid == 0)
  {
    //process 2 does not need to write anything
    close(fds[1]);
    //process 2 needs to read from the read end of the pipe
    //associate read end of the pipe with STDIN of the first process
    dup2(fds[0], STDIN_FILENO);
    close(fds[0]);
    execvp(argv2[0], argv2);
  }
  close(fds[0]);
  //close(fds[1]);
}
