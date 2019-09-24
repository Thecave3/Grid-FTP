//
// Created by Andrea Lacava on 20/09/19.
//

#include <unistd.h>
#include "protocol.h"

int server_init(int server_port) {
    int sock_desc, ret;
    struct sockaddr_in *serv_addr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(sock_desc, "Error on socket creation", TRUE);

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr->sin_port = htons(server_port);

    ret = bind(sock_desc, (struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Error on socket binding", TRUE);

    ret = listen(sock_desc, SOMAXCONN);
    ERROR_HELPER(ret, "Error on socket listening", TRUE);

    return sock_desc;
}

ssize_t recv_message(int socket_desc, char *buffer) {
    int ret;
    int bytes_read = 0;

    while (1) {
        ret = recv(socket_desc, buffer + bytes_read, 1, 0);
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            else
                ERROR_HELPER(ret, "Fatal error in read data, panic", TRUE);
        }

        if (ret == 0)
            break; // TCP connection close by one side
        if (buffer[bytes_read] == '\n')
            break;

        bytes_read++;
    }

    return bytes_read;
}

ssize_t send_message(int socket_desc, char *buffer, int msg_length) {
    int ret;
    int bytes_written = 0;

    while (1) {
        ret = send(socket_desc, buffer + bytes_written, msg_length, 0);
        if (ret == -1) {
            if (errno == EINTR) {
                perror("Error in data write, I am going to repeat\n");
                continue;
            }
            ERROR_HELPER(ret, "Fatal error in write data, panic", TRUE);
        } else if ((bytes_written += ret) == msg_length) break;
    }

    return bytes_written;
}

void craft_ack_response_stub(char *buffer) {
    memset(buffer, 0, strlen(buffer));
    strncpy(buffer, OK_RESPONSE, strlen(OK_RESPONSE));
    strncat(buffer, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
}

void craft_nack_response(char *buffer) {
    memset(buffer, 0, strlen(buffer));
    strncpy(buffer, NOK_RESPONSE, strlen(NOK_RESPONSE));
    strncat(buffer, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
}