#ifndef _INODE_H
#define _INODE_H

#include "unixfilesystem.h"

// each inode is 32 bytes, one block can hold 512 /32 or 16 inodes
#define INODESPERBLOCK (DISKIMG_SECTOR_SIZE / sizeof(struct inode))
// single level of indirection, maximum number of payload blocks is: 7
// blocks * 256 Payblocks per block = 1792
#define MAXNUMPAYLOADBLOCKSLEVEL1 7
#define MAXPAYBLOCKSLEVEL1 (MAXNUMPAYLOADBLOCKSLEVEL1 * DISKIMG_SECTOR_SIZE / 2)
#define LASTIADDR 7
#define BLOCKNUMBERSPERBLOCK (DISKIMG_SECTOR_SIZE / 2)
#define ERROR -1
/**
 * Fetches the specified inode from the filesystem. 
 * Returns 0 on success, -1 on error.  
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp); 

/**
 * Given an index of a file block, retrieves the file's actual block number
 * of from the given inode.
 *
 * Returns the disk block number on success, -1 on error.  
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum);

/**
 * Computes the size in bytes of the file identified by the given inode
 */
int inode_getsize(struct inode *inp);

#endif // _INODE_
