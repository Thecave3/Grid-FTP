//
// Created by Andrea Lacava on 20/09/19.
//

#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "protocol.h"

/**
 *
 * Initialization of a server.
 *
 * @param server_port port in which the server has to be set up
 * @return the descriptor active of the server
 */
int server_init(int server_port) {
    int sock_desc, ret, tr = 1;
    struct sockaddr_in *serv_addr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(sock_desc, "Error on socket creation", TRUE);

    // kill "Address already in use" error message
    if (setsockopt(sock_desc, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr->sin_port = htons(server_port);

    ret = bind(sock_desc, (struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Error on socket binding", TRUE);

    ret = listen(sock_desc, SOMAXCONN);
    ERROR_HELPER(ret, "Error on socket listening", TRUE);

    return sock_desc;
}


/**
 * Receive a message
 *
 * @param socket_desc descriptor of the socket connected
 * @param buffer in which data has to be put
 * @return the number of bytes read
 */
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
    //printf("Received message: \"%s\"\n", buffer);
    return bytes_read;
}

/**
 * Sends a message.
 *
 * @param socket_desc destination of the message
 * @param buffer where the content of the message is
 * @param msg_length length of the message
 * @return the number of bytes written
 */
ssize_t send_message(int socket_desc, char *buffer, unsigned long msg_length) {
    //printf("I am sending \"%s\"\n", buffer);
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

/**
 * Receive a file from socket_desc with name file_name and size file_size
 *
 * @param socket_desc source of the file
 * @param file_name name of the file
 * @param file_size size of the file
 * @return pointer to the file, NULL in case of errors
 */
FILE *recv_file(int socket_desc, char *file_name, long unsigned file_size) {
    int ret;
    char buf[BUFSIZ];
    FILE *fp;
    if (!(fp = fopen(file_name, "wb"))) {
        fprintf(stderr,
                "Can't open file at \"%s\". Wrong path?\n", file_name);
        exit(EXIT_FAILURE);
    }
    long unsigned remain_data = file_size;

    while ((remain_data > 0) && ((ret = recv(socket_desc, buf, BUFSIZ, 0)) > 0)) {
        fwrite(buf, sizeof(char), ret, fp);
        remain_data -= ret;
        fprintf(stdout, "Received %d bytes, remaining :- %lu bytes\n", ret, remain_data);
    }
    fclose(fp);

    return fp;
}

/**
 * Sends a file
 *
 * @param socket_desc destination of the file
 * @param file_path name of the file
 * @param file_size size of the file
 */
void send_file(int socket_desc, char *file_path, unsigned long file_size) {
    int fd, ret;

    if (!(fd = open(file_path, O_RDONLY))) {
        fprintf(stderr, "Can't open file at \"%s\". Wrong path?\n", file_path);
        exit(EXIT_FAILURE);
    }

    long unsigned remain_data = file_size;

    while (remain_data) {
        ret = sendfile(socket_desc, fd, NULL, BUFSIZ);
        if (ret == -1) {
            if (errno == EINTR) {
                perror("Error in data write, I am going to repeat\n");
                continue;
            }
            ERROR_HELPER(ret, "Fatal error in write data, panic", TRUE);
        }
        remain_data -= ret;
    }
    close(fd);
}

/**
 * Creates the protocol basic header.
 *
 * @param buffer in which the data has to be put in
 * @param command to be put in
 * @param buffer_size size of the buffer allocated
 */
void craft_header(char *buffer, char *command, size_t buffer_size) {
    memset(buffer, 0, buffer_size);
    strncpy(buffer, command, strlen(command));
}

/**
 * Creates a complete command request with no args.
 *
 * @param buffer in which the data has to be put in
 * @param command to be put in
 * @param buffer_size size of the buffer
 */
void craft_request(char *buffer, char *command, size_t buffer_size) {
    craft_header(buffer, command, buffer_size);
    strncat(buffer, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
}

/**
 * Create a command request that has to be concluded by user with args.
 *
 * @param buffer in which the data has to be put in
 * @param command to be put in
 * @param buffer_size size of the buffer
 */
void craft_request_header(char *buffer, char *command, size_t buffer_size) {
    craft_header(buffer, command, buffer_size);
    strncat(buffer, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
}

/**
 * Create a basic header for positive response.
 *
 * @param buffer in which the data has to be put in
 * @param buffer_size size of the buffer
 */
void craft_ack_stub(char *buffer, size_t buffer_size) {
    craft_header(buffer, OK_RESPONSE, buffer_size);
}

/**
 * Create a basic header for positive response that has to be completed by user.
 *
 * @param buffer in which the data has to be put in
 * @param buffer_size size of the buffer
 */
void craft_ack_response_header(char *buffer, size_t buffer_size) {
    craft_ack_stub(buffer, buffer_size);
    strncat(buffer, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
}

/**
 * Create a complete positive response ready to be sent.
 *
 * @param buffer in which the data has to be put in
 * @param buffer_size size of the buffer
 */
void craft_ack_response(char *buffer, size_t buffer_size) {
    craft_ack_stub(buffer, buffer_size);
    strncat(buffer, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
}

/**
 * Create a complete negative response ready to be sent.
 *
 * @param buffer in which the data has to be put in
 * @param buffer_size size of the buffer
 */
void craft_nack_response(char *buffer, size_t buffer_size) {
    craft_header(buffer, NOK_RESPONSE, buffer_size);
    strncat(buffer, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
}