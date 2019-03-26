#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

#include "unixfilesystem.h"
#include "direntv6.h"

#define CAPLENGTH 14
#define BLOCKNUMPERBLOCK (DISKIMG_SECTOR_SIZE / sizeof(struct inode))
#define NOTFOUND -1

/**
 * Looks up the specified name (name) in the specified directory (dirinumber).  
 * If found, return the directory entry in space addressed by dirEnt.  Returns 0 
 * on success and something negative on failure. 
 */
int directory_findname(struct unixfilesystem *fs, const char *name,
                       int dirinumber, struct direntv6 *dirEnt);

#endif // _DIECTORY_H_
