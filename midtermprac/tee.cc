#include "stdio.h"

// [0 1 2 3 ... n -1]
// [0, 2, 4, ....n-2]
// length = n;
// length = 6;
// [0, 1, 2, 3, 4, 5]
// [0, 2, 4]

// tee function accepts an array of length 3 or more via fds, and the
// length of that array is passed through via n, tee then populates fd[0]
// through fds[n-1] with valid file descriptors such that eachof fds[1],
// fds[2], etc. through fds[n-1] read independent copies of each and every
// character written to fds[0]
//length = 6: 0, 2, 4, 6 - 1 = 5
void tee(int fds[], int n)
{
  int array_len = 2 * n;
  int fdt[array_len];
  //pipe on even indices
  for (int i = 0 ; i < n - 1; i+=2)
  {
    pipe(fdt + i);
  }
  //populate fds[0] with write endpoint of first pipe
  fds[0] = fdt[0];
  //populate all other slots of fds with read ends of other pipes
  for (int i = 1; i < n)
  {
    fds[i] = fdt[2*i];
  }
  pid_t pid = fork();
  if (pid == 0)
  {

    dup2(fds[0], STDIN_FILENO);

    dup2(fds[i], STDOUT_FILENO);
  }

}

int main(int argc, char *argv[])
{
  int fds[10];
  tee(fds, 10);
  write(fds[0], "a", 1);
  for (size_t i = 1; i < 10; i++)
  {
    char ch;
    read(fds[i], &ch, 1);
    printf("%c", ch + i);
  }
  printf("\n");
  return 0;
}
