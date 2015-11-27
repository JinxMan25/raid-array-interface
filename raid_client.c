////////////////////////////////////////////////////////////////////////////////
//
//  File          : raid_client.c
//  Description   : This is the client side of the RAID communication protocol.
//
//  Author        : ????
//  Last Modified : ????
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// Project Include Files
#include <raid_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

// Global data
unsigned char *raid_network_address = RAID_DEFAULT_IP; // Address of CRUD server
unsigned short raid_network_port = RAID_DEFAULT_PORT; // Port of CRUD server

struct socketData {
  RAIDOpCode opCode;
  int Length;
  void * Data;
};

int64_t socketfd, length, lengthNBO;

void close_connection() {
  close(socketfd);
}

int establish_connection() {
  struct sockaddr_in caddr; 

  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(raid_network_port);


  //ASCII to Network which converts the dotted string to network value in the struct
  inet_aton((const char*)raid_network_address, &(caddr.sin_addr));

  socketfd = socket(AF_INET, SOCK_STREAM, 0);

  if (connect(socketfd, (const struct sockaddr *)&caddr, sizeof(struct sockaddr)) == -1) {
    return -1;
  }
}

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_raid_bus_request
// Description  : This the client operation that sends a request to the RAID
//                server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : op - the request opcode for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

RAIDOpCode client_raid_bus_request(RAIDOpCode op, void *buf) {

  length = sizeof(buf);


  op = htonll64(op);

  /*data.opCode = op;
  data.Length = length;
  data.Data = &buf;*/
 
  //Send opcode and get a response from server
  if (send(socketfd, &op, sizeof(op), 0) != sizeof(op)) {
     logMessage(LOG_ERROR_LEVEL, "Opcode send failed!");
     return -1;
  }
 
   logMessage(LOG_INFO_LEVEL, "Raid OpCode SENT!");
 
  /*if (recv(socketfd, &op, sizeof(op), 0) < 0) {
     logMessage(LOG_ERROR_LEVEL, "Opcode receive failed!");
  }*/
  logMessage(LOG_INFO_LEVEL, "Waiting");

  if (buf == 0){
    length = 0;
  }

  lengthNBO = htonll64(length);

  //Send the length and get a response from server
  if (send(socketfd, &lengthNBO, sizeof(lengthNBO), 0) != sizeof(lengthNBO)) {
    logMessage(LOG_ERROR_LEVEL, "Send 'length' failed!");
    return -1;
  }

  logMessage(LOG_INFO_LEVEL, "Length sent!");
 
  /*if (recv(socketfd, &length, sizeof(length), 0) < 0) {
 	logMessage(LOG_ERROR_LEVEL, "Receive from length failed!");
   return -1;
  }*/
 
  //if length is not zero, send the buffer to the server
  if (length != 0) {

    if (send(socketfd, (char*)buf, sizeof(buf), 0) != sizeof(buf)) {
      logMessage(LOG_ERROR_LEVEL, "Buffer send failed!");
      return -1;
    }

    //logMessage(LOG_INFO_LEVEL, "waiting on buffer response!");

    /*if (recv(socketfd, (char*)buf, sizeof(buf), 0) < 0) {
      logMessage(LOG_ERROR_LEVEL, "Buffer receive failed!");
      return -1;
    }*/

 }

  op = ntohll64(op);

  return op;
}

