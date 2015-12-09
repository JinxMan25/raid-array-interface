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
#include <tagline_driver.h>
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

int64_t socketfd, length, lengthNBO, recvLength;

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
  return 1;
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

  if (extract_raid_response(op, "REQUEST_TYPE") == RAID_INIT){
    establish_connection();
  }
  int blocks = extract_raid_response(op, "BLOCKS");

  length = blocks*RAID_BLOCK_SIZE;

  op = htonll64(op);
  lengthNBO = htonll64(length);

  //Send opcode and get a response from server
  if (write (socketfd, &op, sizeof(op)) != sizeof(op)){
    logMessage(LOG_ERROR_LEVEL, "Opcode send failed!");
    return -1;
  }

  //Send the length and get a response from server
  if (write(socketfd, &lengthNBO, sizeof(lengthNBO)) != sizeof(lengthNBO)) {
    logMessage(LOG_ERROR_LEVEL, "Send 'length' failed!");
    return -1;
  }

  logMessage(LOG_INFO_LEVEL, "Sent length!");

  if (recv(socketfd, &lengthNBO, sizeof(lengthNBO), 0) < 0) {
    logMessage(LOG_ERROR_LEVEL, "Receive from length failed!");
    return -1;
  }

  recvLength = ntohll64(lengthNBO);

  //logMessage(LOG_INFO_LEVEL, "Received length from server!");

  if (extract_raid_response(ntohll64(op), "REQUEST_TYPE") == RAID_READ) {
    logMessage(LOG_INFO_LEVEL, "Trying to receive buffer from server!");
    if (read(socketfd, (char*)buf, sizeof(buf)) != sizeof(buf)) {
      logMessage(LOG_ERROR_LEVEL, "Buffer receive failed!");
      return -1;
    }
    logMessage(LOG_INFO_LEVEL, "Buffer read into 'buf'!");
  }

  if (extract_raid_response(ntohll64(op), "REQUEST_TYPE") == RAID_WRITE) {
    logMessage(LOG_INFO_LEVEL, "Trying to write buffer to server!");
    if (write(socketfd, (char*)buf, sizeof(buf)) != sizeof(buf)) {
      logMessage(LOG_ERROR_LEVEL, "Buffer send failed!");
      return -1;
    }
    logMessage(LOG_INFO_LEVEL, "Buffer written!");
  }

  logMessage(LOG_INFO_LEVEL, "Waiting...");


  op = ntohll64(op);

  if (extract_raid_response(op, "REQUEST_TYPE") == RAID_CLOSE){
    close_connection();
  }

  return op;
}
