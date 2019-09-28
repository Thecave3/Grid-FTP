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
#define TRANSFER_FROM_DR_CMD "dr_transfer"
#define LS_CMD "ls"

#define QUIT_CMD "quit"

#define COMMAND_DELIMITER ","
#define COMMAND_TERMINATOR "\n"

#define FILE_SIZE_LIMIT 20
// it means that protocol can move files with at maximum FILE_SIZE_LIMIT digits (e.g. a 100 bytes file has 3 digits)
// This limit was put just to optimize memory allocated in for file size buffer since with a number long 20 digits
// it is possible to represent 8 EB which is the maximum theoretical file size on Linux NFSv3 (client side)
// https://www.novell.com/documentation/suse91/suselinux-adminguide/html/apas04.html

int server_init(int server_port);

ssize_t recv_message(int socket_desc, char *buffer);

ssize_t send_message(int socket_desc, char *buffer, unsigned long msg_length);

FILE *recv_file(int socket_desc, char *file_name, unsigned long file_size);

void send_file(int socket_desc, char *file_path, unsigned long file_size);

void craft_request_header(char *buffer, char *command);

void craft_ack_response_header(char *buffer);

void craft_ack_response(char *buffer);

void craft_nack_response(char *buffer);

#endif //GRID_FTP_PROTOCOL_H
