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
 struct sockaddr_in caddr; 
 struct socketData data;
 struct socketData resp;
 int length;
 int64_t, socketfd, length;

 length = sizeof(buf);

 caddr.sin_family = AF_INET;
 caddr.sin_port = htons(raid_network_port);


 //ASCII to Network which converts the dotted string to network value in the struct
 inet_aton((const char*)raid_network_address, &(caddr.sin_addr));

 socketfd = socket(AF_INET, SOCK_STREAM, 0);

 if (connect(socketfd, (const struct sockaddr *)&caddr, sizeof(struct sockaddr)) == -1) {
    return -1;
 }

 op = htonll64(op);
 length = htonll64(length);

 /*data.opCode = op;
 data.Length = length;
 data.Data = &buf;*/

 send(socketfd, &op, sizeof(data), 0);

 recv(socketfd, &resp, sizeof(resp), 0);

 if (sizeof(buf) != 0) {
    send(socketfd, &op, sizeof(data), 0);

    recv(socketfd, &resp, sizeof(resp), 0);

    send(socketfd, &op, sizeof(data), 0);

    recv(socketfd, &resp, sizeof(resp), 0);
 }

 data.opCode = ntohll64(resp.opCode);


  return data.opCode;

}

