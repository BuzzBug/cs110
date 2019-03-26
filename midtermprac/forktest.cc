#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static int counter = 0;
int main(int argc, char *argv[])
{
  for (int i = 0; i < 2; i++)
  {
    fork();
    counter++;
    printf("counter = %d\n", counter);
  }
  printf("counter = %d\n", counter);
  return 0;
}
