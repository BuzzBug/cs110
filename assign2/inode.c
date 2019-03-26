#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"
#include "ino.h"
#include "unixfilesystem.h"

int inumber_index(int inumber)
{
  return (inumber - ROOT_INUMBER);
}


int inumberind_2_blockind(int inumber)
{
  int inumindex = inumber_index(inumber);
  return inumindex / INODESPERBLOCK + INODE_START_SECTOR;
}


int inodeindex_in_block(int inumber)
{
  return inumber_index(inumber) % INODESPERBLOCK;
}

int PayBlocksPerFile(struct inode *inp)
{
  int file_bytes = inode_getsize(inp);
  int num_blocks = file_bytes / DISKIMG_SECTOR_SIZE;
  int remaining_bytes = file_bytes % DISKIMG_SECTOR_SIZE;
  if (remaining_bytes != 0)
  {
    num_blocks++;
  }
  return num_blocks;
}

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
  // get blockNumber that contains the desired inode; subtract
  // root_inumber from inumber so division by inodenumperblock would
  // automatically round down to the correct block offset
  int blockNumber = inumberind_2_blockind(inumber);
  // create an array of inodes 
  struct inode inode_arr[INODESPERBLOCK]; 
  // get the inode out of the block
  int bytesread = diskimg_readsector(fs->dfd, blockNumber, inode_arr);
  // check if reading from disk was successful  
  if (bytesread == ERROR)
  {
    printf("ReadError!\n");
    return ERROR;
  }
  // copy the relevant inode using the zero-indexed inumber 
  int blockIndexOffset = inodeindex_in_block(inumber);
  *inp = inode_arr[blockIndexOffset];
  return 0;
}

int getBlockNumInInode(int blockNum, struct inode *inp)
{
  int index = blockNum / BLOCKNUMBERSPERBLOCK;
  return inp->i_addr[index];
}

int getBlockNumInIndBlock(int blockNum, void *buf)
{
  return *((uint16_t *)buf + (blockNum % BLOCKNUMBERSPERBLOCK));
}

int large_file_index(int blockNum, struct unixfilesystem *fs, struct inode *inp)
{
  // 2 levels of indirection
  char buf[DISKIMG_SECTOR_SIZE];
  // read block whose block number is stored in the 8th slot, doubly
  // indirect block
  int numbytesread = diskimg_readsector(fs->dfd, inp->i_addr[LASTIADDR], buf);
  if (numbytesread == ERROR)
  {
    return ERROR;
  }
  // get the number of blocks left over from first seven blocks of
  // indirect blocks, this will tell you which entry in indirect block
  // you should enter;
  int remainingpayload = blockNum - MAXPAYBLOCKSLEVEL1;
  int blockindneeded = remainingpayload / BLOCKNUMBERSPERBLOCK;
  // cast the char buffer as 2-byte ints and add the offset,
  // dereference this pointer to get the number itself
  int indirectblockNum = *((uint16_t *)buf + blockindneeded);
  return indirectblockNum;
}

// remove the placeholder implementation and replace with your own
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    
  int num_blocks = PayBlocksPerFile(inp);

  if ((inp->i_mode & ILARG) == 0)
  {
    return inp->i_addr[blockNum];
  }
  // single level of indirection:
  int indblockNum = 0;
  if (blockNum < MAXPAYBLOCKSLEVEL1)
  {
    // find the index of the indirect block that contains the blockNum index 
    indblockNum = getBlockNumInInode(blockNum, inp);
  }
  else
  {
    indblockNum = large_file_index(blockNum, fs, inp);
    if (indblockNum == ERROR)
    {
      printf("Block access error\n");
      return ERROR;
    }
    int remainingpayload = blockNum - MAXPAYBLOCKSLEVEL1;
    blockNum = remainingpayload;
  }
    // read this indirect block out of memory
  char buf[DISKIMG_SECTOR_SIZE];
  int numbytesread = diskimg_readsector(fs->dfd, indblockNum, buf);
  if (numbytesread == ERROR)
  {
    printf("Read Error\n");
    return ERROR;
  }    
  return getBlockNumInIndBlock(blockNum, buf);
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
