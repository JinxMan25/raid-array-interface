///////////////////////////////////////////////////////////////////////////////
//
//  File           : tagline_driver.c
//  Description    : This is the implementation of the driver interface
//                   between the OS and the low-level hardware.
//
//  Author         : ?????
//  Created        : ?????

// Include Files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project Includes
#include "raid_bus.h"
#include "tagline_driver.h"


//This is the structure for a raid disk, and 'container' contains the 8192 blocks
struct tagline {
  int **taglineBlocks;
} *taglines;

struct raid_disks {
  int currentSize;
};

//Storing round robin 
int currentDisk = 0;
int gmaxLines;

struct raid_disks disks[RAID_DISKS];


//Globals
//initialize 5 structs for holding 5 disks
uint32_t respOpCode[6];

////////////////////////////////////////////////////////////////////////////////
//
// Function     :extract_raid_response
// Description  : extracts the response opcode returned from raid_bus_request
//
// Inputs       : resp which is of type RAIDOpCode, and buf
// Outputs      : returns void because it sets values in the global variable

void extract_raid_response(RAIDOpCode resp) {

  uint32_t block_ID = resp;
  respOpCode[5] = block_ID;
  resp >>= 32;

  uint32_t status = resp&0x01;
  respOpCode[4] = status;
  resp >>= 1;

  uint32_t unused = resp&0x7f;
  respOpCode[3] = unused;
  resp >>= 7;

  uint32_t disk_number = resp&0xff;
  respOpCode[2] = disk_number;
  resp >>= 8;

  uint32_t num_blocks = resp&0xff;
  respOpCode[1] = num_blocks;
  resp >>= 8;

  uint32_t request_type = resp&0xff;
  respOpCode[0] = request_type;

}

// Functions
//
////////////////////////////////////////////////////////////////////////////////
//
// Function     : status_check_helper
// Description  : checks if status of the operation being performed succeeds or not
//
// Inputs       : request_type: takes in response opcode from raid_bus_request and a char pointer to denote what operation it is
// Outputs      : returns an int indicating success/failure (0 = operation successful! And 1 = operation failed!)

int status_check_helper(RAIDOpCode response, char *op){
  int boolean = 0;

    extract_raid_response(response);

    //if status is not equal to 0, that means the operation has failed
    if (respOpCode[4] != 0) {
      boolean = 1;
      logMessage(LOG_INFO_LEVEL, "%s HAS FAILED!\n", op);
    }
    return boolean;
}



////////////////////////////////////////////////////////////////////////////////
//
// Function     :create_raid_request
// Description  : Packs the the all the parameters into a 64 bit integer
//
// Inputs       : request_type: operation to pass to the bus; num_blocks:number of blocks to write to the raid array; disk_number: which raid array disk to write to; unused: TODO; status: success or failure of the operation; block_ID: starting block number to start writing to
// Outputs      : returns the packed 64 bit opcode

RAIDOpCode create_raid_request(uint64_t request_type, uint64_t num_blocks, uint64_t disk_number, uint64_t unused, uint64_t status, uint64_t block_ID) {

  uint64_t requestType_bits, num_block_bits, disk_number_bits, unused_bits, status_bits, block_ID_bits;

  RAIDOpCode packedOpCode;

  requestType_bits = request_type << 56;
  num_block_bits = num_blocks << 48;
  disk_number_bits = disk_number << 40;
  unused_bits = unused << 33;
  status_bits = status << 32;
  block_ID_bits = block_ID;

  packedOpCode = requestType_bits | num_block_bits | disk_number_bits | unused_bits | status_bits | block_ID_bits;

  return packedOpCode;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_driver_init
// Description  : Initialize the driver with a number of maximum lines to process
//
// Inputs       : maxlines - the maximum number of tag lines in the system
// Outputs      : 0 if successful, -1 if failure
  
int tagline_driver_init(uint32_t maxlines) {

  //assign global var 'gmaxlines' to maxlines so that it can be used in raid_disk_signal()
  gmaxLines = maxlines;
  int i,j;
  RAIDOpCode respInit, respFormat;

  //Just initialize each array of disks to RAID_DISKBLOCKS
  for (i = 0; i < RAID_DISKS; i++) {
    disks[i].currentSize = RAID_DISKBLOCKS;
  }

  //Dynamically allocate memory for maxlines of taglines
  taglines = malloc(maxlines * sizeof(struct tagline));

  //For each tagline, dynamically allocate memory for max taglineblocks to store information about the disk and block in the disk
  for (i = 0; i < maxlines; i++) {
    taglines[i].taglineBlocks = malloc(MAX_TAGLINE_BLOCK_NUMBER * sizeof(int*));
    //For each tagline block, dynamically allocation an array of 4 integers. First two will hold primary disk and primary disk block and last two will hold backup disk and backup disk block
    for (j = 0; j < MAX_TAGLINE_BLOCK_NUMBER; j++){
      taglines[i].taglineBlocks[j] = malloc (4 * sizeof(int));
      taglines[i].taglineBlocks[j][0] = -1;                            //Initialize them to -1 so that when reading through the data structure, program knows whether this taglineblock was written to or not
      taglines[i].taglineBlocks[j][2] = -1;
    }
  }
  
  //Initializes the raid arrays
  respInit = raid_bus_request(create_raid_request(RAID_INIT, RAID_DISKBLOCKS/RAID_TRACK_BLOCKS, RAID_DISKS, 0, 0, 0), NULL);

  //check if init fails or not
  if (status_check_helper(respInit, "INIT")){
    return -1;
  }

  //Formats the disks
  for (i = 0;i < RAID_DISKS; i++){
    respFormat = raid_bus_request(create_raid_request(RAID_FORMAT, RAID_DISKBLOCKS/RAID_TRACK_BLOCKS, i, 0, 0, 0), NULL);
    
    //check if succeeded or not!
    if (status_check_helper(respFormat, "FORMAT")){
      return -1;
    }
  }

	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE: initialized storage (maxline=%u)", maxlines);
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_read
// Description  : Read a number of blocks from the tagline driver
//
// Inputs       : tag - the number of the tagline to read from
//                bnum - the starting block to read from
//                blks - the number of blocks to read
//                bug - memory block to read the blocks into
// Outputs      : 0 if successful, -1 if failure

int tagline_read(TagLineNumber tag, TagLineBlockNumber bnum, uint8_t blks, char *buf) {

  RAIDOpCode readResp;
  int i;
  int primaryDisk, primaryDiskBlock;

  //For each number of blks, i, access the tagline by 'tab' and taglineblock by 'bnum+i' to fetch primary disk and primary disk block to read from
  for (i=0;i<blks;i++) {

    primaryDisk = taglines[tag].taglineBlocks[bnum + i][0];             //Fetch primary disk
    primaryDiskBlock = taglines[tag].taglineBlocks[bnum + i][1];        //Fetch primary disk block

    logMessage(LOG_INFO_LEVEL, "Trying to read Disk : %d  Block: %d", primaryDisk, primaryDiskBlock);

    //Call the raid bus to read the buffer into 'buf' in 1024 chunks
    readResp = raid_bus_request(create_raid_request(RAID_READ, 1, primaryDisk, 0, 0, primaryDiskBlock), &buf[i*RAID_BLOCK_SIZE]);
    if (status_check_helper(readResp, "READ")){
      return -1;
    }
  }

	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE : read %u blocks from tagline %u, starting block %u.",
			blks, tag, bnum);
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : raid_disk_signal
// Description  : Upon signaling disk failure, checks whether each disk failed or not, then formats the disk and recovers the number of blocks written to the failed disk of the failed disk
//
// Inputs       : void
// Outputs      : 0 if successful, -1 if failure


int raid_disk_signal(){
  //Declarations -> 
  // 'i' iterates over each disks to check if it failed or not
  // 'j' iterates over the number of blocks that was written to the failed disk (RAID_DISKBLOCKS - disks[i].currentSize)
  // 'x' iterates over the maxlines of taglines
  // 'y' iterates over the taglineblocks of each taglines

  int i,j, x, y, primaryDisk, primaryDiskBlock, backUpDisk, backUpDiskBlock, failedDiskCurrentSize, found;
  failedDiskCurrentSize = 0;
  char buf[RAID_BLOCK_SIZE];
  RAIDOpCode statusResp, formatResp, readResp, writeResp;

  //Check each disk if it failed or not
  for (i = 0; i < RAID_DISKS; i++) {
    statusResp = raid_bus_request(create_raid_request(RAID_STATUS, 0, i, 0, 0, 0), NULL);
    extract_raid_response(statusResp);
    // if disk fails, format the disk 
    if (respOpCode[5] == RAID_DISK_FAILED){
      formatResp = raid_bus_request(create_raid_request(RAID_FORMAT, RAID_DISKBLOCKS/RAID_TRACK_BLOCKS, i, 0, 0, 0), buf);
      //if it is successful, go through each block of the disk that was written to and brute force search against the tagline data mapping
      if (status_check_helper(formatResp, "Format disk")){
        return 1;
      } else {

        //Go through each block of the failed disk that was written to, to search in the tagline data structure and recover
        for (j= 0; j < (RAID_DISKBLOCKS - disks[i].currentSize);j++){

          //For Each block in the failed disk. Check if it exists while iterating through the tagline mapping
          for (x = 0; x < gmaxLines; x++) {
            //Outer loop to iterate through maxlines of taglines
            for (y = 0; y < MAX_TAGLINE_BLOCK_NUMBER;y++) {
              //Inner loop to iterate through MAX_TAGLINE_BLOCKS of each tagline
              //
              //Get backup block
              primaryDisk = taglines[x].taglineBlocks[y][0];
              primaryDiskBlock = taglines[x].taglineBlocks[y][1];
              backUpDisk = taglines[x].taglineBlocks[y][2];
              backUpDiskBlock = taglines[x].taglineBlocks[y][3];

              //Checck if primaryDisk and the corresponding block fethed from the tagline datastructure is equal to the disk and block to recover..same for backup because I'm storing backups on every disk, so I have to recover back ups as well
              if (((primaryDisk > -1) && (primaryDisk == i) && (primaryDiskBlock == j)) || ((backUpDisk > -1) && (backUpDisk == i) && (backUpDiskBlock == j))){


                logMessage(LOG_INFO_LEVEL, "Found!");
                found = 1;

                //if primary disk and block was found, then read from the corresponding backup disk and backup block else just read from corresponding primary disk and block
                if ((primaryDisk == i) && (primaryDiskBlock == j)){
                  readResp = raid_bus_request(create_raid_request(RAID_READ, 1, backUpDisk, 0, 0, backUpDiskBlock), buf);
                } else {
                  readResp = raid_bus_request(create_raid_request(RAID_READ, 1, primaryDisk, 0, 0, primaryDiskBlock), buf);
                }

                // if read into the buffer was succesfull, write the buffer to the failed disk and block to recover
                if (status_check_helper(readResp, "READ Disk FOR WRITE ON FAILED DISK")){
                  return 1;
                } else {

                  logMessage(LOG_INFO_LEVEL, "Recovering diskblock...");
                  writeResp = raid_bus_request(create_raid_request(RAID_WRITE, 1, i, 0, 0, j), buf);
                  if (status_check_helper(writeResp, "WRITE TO FAILED DISK")){
                  } else {
                    logMessage(LOG_INFO_LEVEL, "Recovered DiskBlock!");
                    failedDiskCurrentSize++;
                  }
                }
              } else {
                //Not found in tagline
                //logMessage(LOG_INFO_LEVEL, "Fresh write to block in Primary Disk");
              }
            }
          }
        }
          //If not found, then something went wrong in the mapping in the first place!
          if (found == 0){
            logMessage(LOG_INFO_LEVEL, "Not found in mapping!");
            return 1;
          }
      }
    }

  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_write
// Description  :  Write a number of blocks to raid disks in round-robin fashion
//
// Inputs       : tag - the number of the tagline to store mapping to
//                bnum - the starting block  to store mapping to
//                blks - the number of blocks to write to in the disk
//                buf - memory block to write the blocks into
// Outputs      : 0 if successful, -1 if failure

int tagline_write(TagLineNumber tag, TagLineBlockNumber bnum, uint8_t blks, char *buf) {
  RAIDOpCode writeResOp;
  int hasSpace;

  //checks if write is successfull, if it is, it also checks if the block has been overwritten by by looking at the struct called 'disks' and logs it if it has. If write fails, it logs that it has failed!
  int i = 1;

  //for each block, write to disk in round robin fashion
  for (i=0;i < blks; i++) {
    //keep looping until a disk has space, if not then return -1
    while (1) {
      if (disks[currentDisk].currentSize > 0) {
        hasSpace = 1;
        // if the block hasn't been written to, then its a fresh write
        if (taglines[tag].taglineBlocks[bnum+i][0] == -1) {

          //Write to the 'currentDisk' and the block that is retrieved by RAID_DISKBLOCKS - disks[currentDisk].currentSize)
          writeResOp = raid_bus_request(create_raid_request(RAID_WRITE, 1, currentDisk, 0, 0, RAID_DISKBLOCKS - disks[currentDisk].currentSize), &buf[i*RAID_BLOCK_SIZE]);
          logMessage(LOG_INFO_LEVEL, "TAGLINE : wrote %u block(s) to tagline %u, starting block %u. ",
              i, tag, bnum + i);
          logMessage(LOG_INFO_LEVEL, "Fresh write to block in Primary Disk");
          // if success, then map the 'currentDisk' and block to write to in 'currentDisk' to the tagline data structure
          if (status_check_helper(writeResOp, "Fresh WRITE to Primary Disk")){
            return -1;
          } else {
            taglines[tag].taglineBlocks[bnum + i][0] = currentDisk;
            taglines[tag].taglineBlocks[bnum + i][1] = RAID_DISKBLOCKS - disks[currentDisk].currentSize;
          }


          // This is the backup write
          logMessage(LOG_INFO_LEVEL, "Fresh write to block in Backup Disk");
          writeResOp = raid_bus_request(create_raid_request(RAID_WRITE, 1, (currentDisk == 16) ? 0 : (currentDisk + 1), 0, 0, RAID_DISKBLOCKS - disks[(currentDisk == 16) ? 0 : (currentDisk + 1)].currentSize), &buf[i*RAID_BLOCK_SIZE]);
          if (status_check_helper(writeResOp, "Fresh WRITE to Backup Disk")){
            return -1;
          }  else {
            // This is round robin, so if currentDisk is 16, go to Disk 0 to store backup
            taglines[tag].taglineBlocks[bnum + i][2] = (currentDisk == 16) ? 0 : (currentDisk + 1);
            taglines[tag].taglineBlocks[bnum + i][3] = RAID_DISKBLOCKS - disks[(currentDisk == 16) ? 0 : (currentDisk + 1)].currentSize;
          }

          //on successfull writes, decrease currentsize of disks that the primary block and backup block was written to so that on the next write, it writes to the next block in 'disks[currentDisk]'
          disks[currentDisk].currentSize--;
          disks[(currentDisk == 16) ? 0 : (currentDisk + 1)].currentSize--;


          // This is round-robin so icrement currentDisk to go to the next disk. If it is 16, go to disk 0. Here is it -1 because currentDisk gets increment in the next line
          currentDisk = (currentDisk == 16) ? -1: currentDisk;
          currentDisk++;

          break;
        } else {
          //This is implementation for overwite...no incrementing 'currentDisk' here or decrementing currentSize of 'disks[currentDisk]' beacuse this is an overwrite 
          logMessage(LOG_INFO_LEVEL, "TAGLINE : wrote %u block(s) to tagline %u, starting block %u.\n ",
              i, tag, bnum + i);

          //fetch the primary disk and disk block from tagline data structure to overwrite
          writeResOp = raid_bus_request(create_raid_request(RAID_WRITE, 1, taglines[tag].taglineBlocks[bnum + i][0], 0, 0, taglines[tag].taglineBlocks[bnum + i][1]), &buf[i*RAID_BLOCK_SIZE]);

          if (status_check_helper(writeResOp, "Overwrite to Primary disk")){
            return -1;
          } 

          logMessage(LOG_INFO_LEVEL, "Overwrite to block in Backup Disk");
          //fetch the back up disk and disk block from tagline data structure to overwrite
          writeResOp = raid_bus_request(create_raid_request(RAID_WRITE, 1, taglines[tag].taglineBlocks[bnum + i][2], 0, 0, taglines[tag].taglineBlocks[bnum + i][3]), &buf[i*RAID_BLOCK_SIZE]);
          if (status_check_helper(writeResOp, "Overwrite to Backup disk")){
            return -1;
          } 
          break;
        }
      }

      if (currentDisk == 0) {
        hasSpace = 0;
      }

      //If while loop looped through all disk and is at the end and hasSpace equal 0, that means there was no space in any disk...so return -1
      if (currentDisk == 16 && (hasSpace == 0)){
        return -1;
      }
      currentDisk = (currentDisk >= 16) ? -1 : currentDisk;
      currentDisk++;
    }
  }
  
	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE : wrote %u blocks to tagline %u, starting block %u.",
			blks, tag, bnum + i);
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tagline_close
// Description  : Close the tagline interface
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int tagline_close(void) {
  RAIDOpCode closeResp;
  int i,j;

  //Free the inner most malloc'ed items. Starting with the 4 integers (primDisk & block, backUpDisk & block
  for (i = 0; i < gmaxLines; i++) {
    for (j = 0; j < MAX_TAGLINE_BLOCK_NUMBER;j++){
      free(taglines[i].taglineBlocks[j]);
      taglines[i].taglineBlocks[j] = NULL;
    }
    free(taglines[i].taglineBlocks);
    taglines[i].taglineBlocks = NULL;
  }
  free(taglines);
  taglines = NULL;

  closeResp = raid_bus_request(create_raid_request(RAID_CLOSE, 0, 0, 0, 0, 0),NULL);

  if (status_check_helper(closeResp, "CLOSE")){
    return -1;
  }

	// Return successfully
	logMessage(LOG_INFO_LEVEL, "TAGLINE storage device: closing completed.");
	return(0);
}
