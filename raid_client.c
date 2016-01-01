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
#include <tagline_driver.h>
#include <raid_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>


// Global data
unsigned char *raid_network_address = NULL; // Address of CRUD server
unsigned short raid_network_port = 0; // Port of CRUD server

int64_t socketfd, length, lengthNBO, recvLength;

void close_connection() {
  close(socketfd);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : establish_connection()
// Description  : This creates a socket file descriptor that this client connects, using the RAID_DEFAULT_IP, and RAID_DEFAULT_PORT
//                server.   It will:u
//
// Inputs       : None
//                
// Outputs      : 1 if client has connected succesfully


int establish_connection() {
  struct sockaddr_in caddr; 

  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(RAID_DEFAULT_PORT);

  //ASCII to Network which converts the dotted string to network value in the struct
  inet_aton(RAID_DEFAULT_IP, &(caddr.sin_addr));

  //create file descriptor to connect to socket
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
  int blocks = extract_raid_response(op, "BLOCKS");
  length = blocks*RAID_BLOCK_SIZE;

  if (extract_raid_response(op, "REQUEST_TYPE") == RAID_INIT) {
    establish_connection();
    length = 0;                    //length and blocks are zero for INIT
    blocks = 0;
  }
  if (extract_raid_response(op, "REQUEST_TYPE") == RAID_FORMAT){
    length = 0;
    blocks = 0;
    op = op&0xFF00FFFFFFFFFFFF; //for some reason, my 'block' segment is set to 4, so I set it to 0
  }

  //convert to network byte order so the receiving end can properly decode it and read the right numbers
  op = htonll64(op);
  lengthNBO = htonll64(length);

  //Send opcode and get a response from server
  if (write (socketfd, &op, sizeof(op)) != sizeof(op)){
    logMessage(LOG_ERROR_LEVEL, "Opcode send failed!");
    return -1;
  }
    logMessage(LOG_INFO_LEVEL, "Opcode sent");

  //Send the length to the server to determine whether we need to receive anything from the server
  if (write(socketfd, &lengthNBO, sizeof(lengthNBO)) != sizeof(lengthNBO)) {
    logMessage(LOG_ERROR_LEVEL, "Send 'length' failed!");
    return -1;
  }

    logMessage(LOG_INFO_LEVEL, "Length sent!");

  //send the buffer to the server no matter what. The server will decide whether we'll it'll need it or not
  if (write(socketfd, buf, RAID_BLOCK_SIZE*blocks) != RAID_BLOCK_SIZE*blocks) {
    logMessage(LOG_ERROR_LEVEL, "Send 'buffer' failed!");
    return -1;
  }

    logMessage(LOG_INFO_LEVEL, "Buffer sent!");

  //Then read sequantially, the third read is conditional
  if (read(socketfd, &op, sizeof(op)) != sizeof(op)) {
    logMessage(LOG_ERROR_LEVEL, "Recieve opcode failed");
    return -1;
  }

  logMessage(LOG_INFO_LEVEL, "Opcode received!");

  if (read(socketfd, &lengthNBO, sizeof(lengthNBO)) != sizeof(lengthNBO)) {
    logMessage(LOG_ERROR_LEVEL, "Receive from length failed!");
    return -1;
  }

  logMessage(LOG_INFO_LEVEL, "Length received!");

  //convert  to host byte order to determine whether the buffer needs to be read in
  recvLength = ntohll64(lengthNBO);


  //so if length received from the server is non-zero, then receive a buffer from the server
  if (recvLength != 0) {

    logMessage(LOG_INFO_LEVEL, "Trying to receive buffer from server!");
    if (read(socketfd, buf, recvLength*blocks) != recvLength*blocks) {
      logMessage(LOG_ERROR_LEVEL, "Buffer receive failed!");
      return -1;
    }
    logMessage(LOG_INFO_LEVEL, "Buffer read into 'buf'!");
  }

    
  //convert opcode to host byte order
  op = ntohll64(op);

  // if type if Close, close connection, disconnect from socket
  if (extract_raid_response(op, "REQUEST_TYPE") == RAID_CLOSE){
    close_connection();
  }

  return op;
}
