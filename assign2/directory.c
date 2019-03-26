#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name,
		       int dirinumber, struct direntv6 *dirEnt) {
  
  struct inode inp;
  
  int getflag = inode_iget(fs, dirinumber, &inp);
  if(getflag == ERROR || ((inp.i_mode & IFMT) != IFDIR))
  {
    printf("Failed to access directory\n");
    return ERROR;
  }
  
  //find how many bytes are in the file
  int file_bytes = inode_getsize(&inp);
  
  // find how many blocks are in the file
  int num_dirblocks = file_bytes / DISKIMG_SECTOR_SIZE;
  
  // find how many bytes are in the last unfilled block
  int remainder = file_bytes % DISKIMG_SECTOR_SIZE;
  if (remainder != 0)
  {
    num_dirblocks++;
  }

  // for each block that contains payload, get the block and check if the
  // string is in one of the entries contained in the directory block

  for (int payblock_i = 0; payblock_i < num_dirblocks; payblock_i++)
  {
    char buf[DISKIMG_SECTOR_SIZE];
    int numreadbytes = file_getblock(fs, dirinumber, payblock_i, buf);
    
    if (numreadbytes == ERROR)
    {
      printf("ReadError!\n");
      return ERROR;
    }

    // find how many directory entries are stored in the block
    int numdirentries = numreadbytes / BLOCKNUMPERBLOCK;
    int remainderdirentries = numreadbytes % BLOCKNUMPERBLOCK;
    if (remainderdirentries != 0)
    {
      numdirentries++;
    }
    
    for (int i = 0; i < numdirentries; ++i) {
      struct direntv6 *d = (struct direntv6 *)(buf + i*(sizeof(struct direntv6)));
      if (strncmp(d->d_name, name, CAPLENGTH)== 0)
      {
        //if found, return the directory entry in space addressed by
        //dirEnt.
        *dirEnt = *d;
        return 0;
      }
    }
  }
  return NOTFOUND;    
}
