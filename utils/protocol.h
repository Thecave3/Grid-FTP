//
// Created by Andrea Lacava on 20/09/19.
//

#ifndef GRID_FTP_PROTOCOL_H
#define GRID_FTP_PROTOCOL_H

/**
 * Here are defined all the application layer commands used for communication between agents.
 * Please refer to the protocol documentation to have a full explanation of each string.
 *
*/

#include "error_helper.h"

#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>


#define OK_RESPONSE "ack"
#define NOK_RESPONSE "nack"

#define DR_UPDATE_MAP "update_map\n"

#define CLIENT_AUTH "auth"
#define GET_DR "get_dr"
#define GET "get"
#define PUT "put"
#define REMOVE "remove"
#define TRANSFER "transfer"

int server_init(int server_port);

ssize_t recv_message(int socket_desc, char *buffer);

ssize_t send_message(int socket_desc, char *buffer, int msg_length);

#endif //GRID_FTP_PROTOCOL_H
