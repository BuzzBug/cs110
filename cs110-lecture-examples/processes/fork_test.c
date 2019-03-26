/**
 * File: basic-fork.c
 * ------------------
 * Novelty program to illustrate the basics of fork.  It has the clear flaw
 * that the parent can finish before its child, and the child process isn't
 * reaped by the parent.
 */

#include <stdbool.h>      // for bool
#include <stdio.h>        // for printf
#include <unistd.h>       // for fork, getpid, getppid
//#include "exit-utils.h"   // for our own exitIf

static int counter = 0;
int main(int argc, char *argv[]) {
  pid_t pid = fork();
  if (pid == 0)
  {
    counter += 30;
    printf("counter = %d\n", counter);
  }
  counter++;
  printf("counter = %d\n", counter);
  if (pid != 0)
  {
    waitpid(pid, NULL, 0);
  }
  return 0;
}
//counter = 30
//counter = 1
//counter = 31
