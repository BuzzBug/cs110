#include <stdlib.h>
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
  char *path = strdup(pathname);
  char *pch;
  if (pathname[0] != '/')
  {
    free(path);
    printf("Malformed path name.\n");
    return ERROR;
  }
  else
  {
    struct direntv6 directoryEntry;
    int err;

    pch = strtok(path, "/");
    int directory_inumber = ROOT_INUMBER;
    
    while (pch != NULL)
    {
      err = directory_findname(fs, pch, directory_inumber, &directoryEntry);
      if (err < 0)
      {
        printf("Unable to find directory entry\n");
        return ERROR;
      }
      directory_inumber = directoryEntry.d_inumber;
      pch = strtok(NULL, "/");
    }
    free(path);
    return directory_inumber;
  }
}
