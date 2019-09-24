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

#define DR_UPDATE_MAP_CMD "update_map\n"

#define AUTH_CMD "auth"
#define GET_DR_CMD "get_dr"
#define GET_CMD "get"
#define PUT_CMD "put"
#define REMOVE_CMD "remove"
#define TRANSFER_CMD "transfer"

#define COMMAND_DELIMITER ","
#define COMMAND_TERMINATOR "\n"

int server_init(int server_port);

ssize_t recv_message(int socket_desc, char *buffer);

ssize_t send_message(int socket_desc, char *buffer, int msg_length);



void craft_ack_response_stub(char *buffer);

void craft_nack_response(char *buffer);

#endif //GRID_FTP_PROTOCOL_H
