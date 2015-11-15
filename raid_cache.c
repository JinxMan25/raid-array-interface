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
  int maxSize;
  int lastAccessed;
};

struct block {
  int diskId;
  int blockId;
  char buf[RAID_BLOCK_SIZE];
  int accessCounter;
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

  cache.currentSize = 0; 
  cache.maxSize = max_items; 
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
  int i;
  int low = 999;
  int tracker;

  //from start to for how ever many writes in the cache, look for disk and block and if found, update lastAccessed member and update the buffer in the cache
  for (i = 0; i < cache.currentSize; i++) {
    if (cache.block[i].accessCounter < low) {
      low = cache.block[i].accessCounter;                                      //keep track of the least accessed so that after this iteration, if there is no space in the cache, just replace the least recently used by using the 'tracker' as the index
      tracker = i;
    }
    //if disk and block id found in the cache, update the lastAccessed field and copy over the buffer
    if ((cache.block[i].diskId == dsk) && (cache.block[i].blockId == blk)){
      cache.block[i].accessCounter = cache.lastAccessed;
      memset(cache.block[i].buf, buf, 1024);
    }
  }

  // If no item was found in the cache, and if there is space in the cache, just insert the new disk and block id into the cache
  if ((cache.maxSize - cache.currentSize) > cache.maxSize) {
    cache.block[currentSize + 1].diskId = dsk;
    cache.block[currentSize + 1].blockId = blk;
    cache.block[currentSize + 1].accessCounter = cache.lastAccessed;
    memset(cache.block[currentSize + 1].buf, buf, 1024);

    cache.currentSize++;
  } else {                                            // Else, just freakin' replace the least recently used item in the cache!
    cache.block[tracker].diskId = dsk;
    cache.block[tracker].blockId = dsk;
    memset(cache.block[tracker].buf, buf, 1024);
  }

  cache.lastAccessed++;

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

  for (i = 0; i < cache.currentSize; i++) {
    if ((cache.block[i].diskId == dsk) && (cache.block[i].blockId == blk)) {
      cache.block[i].accessCounter = cache.lastAccessed;
      cache.lastAccessed++;

      return cache.block[i];
    }
  }

  cache.lastAccessed++;
  return NULL;

}

