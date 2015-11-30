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
struct block {
  int diskId;
  int blockId;
  char buf[RAID_BLOCK_SIZE];
  int accessCounter;
};

struct caches {
  struct block *blocks;
  int currentSize;
  int maxSize;
  int lastAccessed;
  int prime;
};


struct caches cache;


int check_prime(int num){
  int i, flag=0;
    for (i=2;i <=num/2; ++i) {
      if (num%i==0) {
        flag=1;
        break;
      }
    }
    if (flag==0) {
      return 1;
    }
    else {
      return -1;
    }
}

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

  cache.blocks = malloc(max_items * sizeof(struct block));

  for (i = 0; i < max_items; i++) {
    cache.blocks[i].diskId = -1;
    cache.blocks[i].blockId = -1;
  }

  i = 0;

  while (1) {
    if (check_prime(max_items - i)) {
      cache.prime = (max_items - i);
      break;
    }
    i++;
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
  int i;

  //for each cache item, set block and disk id to -1, set buf to NULL and accessed times to 0
  for (i = 0; i < cache.currentSize; i++) {
    cache.blocks[i].diskId = -1;
    cache.blocks[i].blockId = -1;
    cache.blocks[i].accessCounter = 0;
   // memset(cache.blocks[i].buf, NULL, 1024);
  }

  cache.lastAccessed = 0;
  cache.currentSize = 0;

	// Return successfully
	return(0);
}


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
  int idx;
  int timeout= 0;
  int low = 999;
  int tracker;

  idx = ((dsk + blk)*(113)) % cache.maxSize;

  //cache eviction, find the least recently used to evict
  if ((cache.maxSize - cache.currentSize) <= 0){
    while (1) {
      if (timeout == cache.maxSize) {
        logMessage(LOG_ERROR_LEVEL, "Too much clusterting");
        return -1;
      }
      if (cache.blocks[idx].diskId == -1) {
        cache.blocks[idx].diskId = dsk;
        cache.blocks[idx].blockId = blk;
        cache.blocks[idx].accessCounter = cache.lastAccessed;
        memset(cache.blocks[idx].buf, *(char*)buf, 1024); 

        cache.currentSize++;
        cache.lastAccessed++;
        break;
      } else if (cache.blocks[idx].diskId == dsk && cache.blocks[idx].blockId == blk) {
        cache.blocks[idx].accessCounter = cache.lastAccessed;
        memset(cache.blocks[idx].buf, *(char*)buf, 1024); 

        cache.lastAccessed++;
        break;
      }
      idx = idx - ((dsk+blk) % cache.prime);
      timeout++;
    }
  } else {

    for (i = 0; i < cache.currentSize; i++) {
      if (cache.blocks[i].accessCounter < low) {
        low = cache.blocks[i].accessCounter;                        //keep track of the least accessed so that after this iteration, if there is no space in the cache, just replace the least recently used by using the 'tracker' as the index
        tracker = i;
      }
    }
    
  }

  // If no item was found in the cache, and if there is space in the cache, just insert the new disk and blocks id into the cache
  if ((cache.maxSize - cache.currentSize) > cache.maxSize) {
    cache.blocks[cache.currentSize + 1].diskId = dsk;
    cache.blocks[cache.currentSize + 1].blockId = blk;
    cache.blocks[cache.currentSize + 1].accessCounter = cache.lastAccessed;
    memset(cache.blocks[cache.currentSize + 1].buf, *(char *)buf, 1024);

    cache.currentSize++;
  } else {                                            // Else, just freakin' replace the least recently used item in the cache!
    cache.blocks[tracker].diskId = dsk;
    cache.blocks[tracker].blockId = dsk;
    memset(cache.blocks[tracker].buf, *(char *)buf, 1024);
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
  int i;

  for (i = 0; i < cache.currentSize; i++) {
    if ((cache.blocks[i].diskId == (int)dsk) && (cache.blocks[i].blockId == (int)blk)) {
      cache.blocks[i].accessCounter = cache.lastAccessed;
      cache.lastAccessed++;

      return (void *)&cache.blocks[i];
    }
  }

  cache.lastAccessed++;
  return NULL;

}

void rehash(){
}

