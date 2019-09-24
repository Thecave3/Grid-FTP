//
// Created by Andrea Lacava on 20/09/19.
//

#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <libgen.h>
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

ssize_t send_message(int socket_desc, char *buffer, unsigned long msg_length) {
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

FILE *recv_file(int socket_desc, char *file_name, long unsigned file_size) {
    char buf[BUFSIZ];
    FILE *fp;
    if (!(fp = fopen(file_name, "wb"))) {
        fprintf(stderr,
                "Can't open userdb at \"%s\". Wrong path?\n", file_name);
        exit(EXIT_FAILURE);
    }
    long unsigned remain_data = file_size;
    int ret;
    while ((remain_data > 0) && ((ret = recv(socket_desc, buf, BUFSIZ, 0)) > 0)) {
        fwrite(buf, sizeof(char), ret, fp);
        remain_data -= ret;
        fprintf(stdout, "Receive %d bytes and we hope :- %lu bytes\n", ret, remain_data);
    }
    fclose(fp);

    return fp;
}

char **get_file_name(char *file_path) {
    // we use a low level operation for file instead of fopen()
    // since sendfile wants a descriptor and not a pointer to file (FILE*).
    struct stat file_stat;
    int fd, ret;
    if (!(fd = open(file_path, O_RDONLY))) {
        fprintf(stderr, "Can't open file at \"%s\". Wrong path?\n", file_path);
        exit(EXIT_FAILURE); // TODO can be improved
    }

    ret = fstat(fd, &file_stat);
    ERROR_HELPER(ret, "Error on retrieving file statistics", TRUE);
    close(fd);

    char *file_size = (char *) malloc(FILE_SIZE_LIMIT * sizeof(char));
    sprintf(file_size, "%ld", file_stat.st_size);

    // From documentation of basename():
    // Both dirname() and basename() may modify the contents of path,
    // so it may be desirable to pass a copy when calling one of these functions.
    char *file_name = basename(strdup(file_path));
    char **result = (char **) malloc(2 * sizeof(char *));

    result[0] = file_name;
    result[1] = file_size;

    return result;
}

void send_file(int socket_desc, char *file_path, char *file_size) {
    char buf[BUFSIZ];
    int fd, ret;

    if (!(fd = open(file_path, O_RDONLY))) {
        fprintf(stderr, "Can't open file at \"%s\". Wrong path?\n", file_path);
        exit(EXIT_FAILURE); // TODO can be improved
    }

    long unsigned remain_data = strtol(file_size, NULL, 10);

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
    free(buf);
}


void craft_ack_stub(char *buffer) {
    memset(buffer, 0, strlen(buffer));
    strncpy(buffer, OK_RESPONSE, strlen(OK_RESPONSE));
}

void craft_ack_response_header(char *buffer) {
    craft_ack_stub(buffer);
    strncat(buffer, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
}

void craft_ack_response(char *buffer) {
    craft_ack_stub(buffer);
    strncat(buffer, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
}

void craft_nack_response(char *buffer) {
    memset(buffer, 0, strlen(buffer));
    strncpy(buffer, NOK_RESPONSE, strlen(NOK_RESPONSE));
    strncat(buffer, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
}