#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

int getvalidbytes(struct unixfilesystem *fs, int inumber, int blockNum, void *buf, struct inode *inp)
{
  int validbytes;
  int file_bytes = inode_getsize(inp);
  // total number of blocks devoted to payload of file
  int numpayblocks = file_bytes / DISKIMG_SECTOR_SIZE;
  // if the index is in the range for full payload blocks, validbytes =
  // 512
  if (blockNum < numpayblocks)
  {
    validbytes = DISKIMG_SECTOR_SIZE;
  }
  // otherwise make sure to calculate the remaining bytes
  else
  {
    int remainder = file_bytes % DISKIMG_SECTOR_SIZE;
    validbytes = remainder;
  }
  return validbytes;
}

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
  struct inode inp;
  int inodeflag = inode_iget(fs, inumber, &inp);
  if (inodeflag == ERROR)
  {
    printf("Inode retrieval error!\n");
    return ERROR;
  }
  int blockNo = inode_indexlookup(fs, &inp, blockNum);
  if (blockNo == ERROR)
  {
    printf("Inode lookup error!\n");
    return ERROR;
  }
  int numbytesread = diskimg_readsector(fs->dfd, blockNo, buf);
  if (numbytesread == ERROR)
  {
    printf("Read error!\n");
    return ERROR;
  }
  return getvalidbytes(fs, inumber, blockNum, buf, &inp);
}

