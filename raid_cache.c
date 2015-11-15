////////////////////////////////////////////////////////////////////////////////
//
//  File           : raid_cache.c
//  Description    : This is the implementation of the cache for the TAGLINE
//                   driver.
//
//  Author         : ????
//  Last Modified  : ????
//

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// Project includes
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <raid_cache.h>

//data structures
struct caches {
  struct block blocks[TAG_CACHE_ITEMS];
  int currentSize;
};

struct block {
  int diskId;
  int blockId;
};

struct caches cache;

//
// TAGLINE Cache interface

////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_raid_cache
// Description  : Initialize the cache and note maximum blocks
//
// Inputs       : max_items - the maximum number of items your cache can hold
// Outputs      : 0 if successful, -1 if failure

int init_raid_cache(uint32_t max_items) {
  int i;

  for (i = 0; i < max_items){
    cache.blocks[i].diskId = -1;
    cache.blocks[i].blockId = -1;
  }

  cache.currentSize = max_items; 
	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : close_raid_cache
// Description  : Clear all of the contents of the cache, cleanup
//
// Inputs       : none
// Outputs      : o if successful, -1 if failure

int close_raid_cache(void) {

	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : returns 
// Description  : Clear all of the contents of the cache, cleanup
//
// Inputs       : none
// Outputs      : o if successful, -1 if failure



////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_raid_cache
// Description  : Put an object into the block cache
//
// Inputs       : dsk - this is the disk number of the block to cache
//                blk - this is the block number of the block to cache
//                buf - the buffer to insert into the cache
// Outputs      : 0 if successful, -1 if failure

int put_raid_cache(RAIDDiskID dsk, RAIDBlockID blk, void *buf) {
  

	// Return successfully
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_raid_cache
// Description  : Get an object from the cache (and return it)
//
// Inputs       : dsk - this is the disk number of the block to find
//                blk - this is the block number of the block to find
// Outputs      : pointer to cached object or NULL if not found

void * get_raid_cache(RAIDDiskID dsk, RAIDBlockID blk) {

	return(????);
}

