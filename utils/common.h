//
// Created by Andrea Lacava on 18/09/19.
//

#ifndef GRID_FTP_UTILS_H
#define GRID_FTP_UTILS_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <pthread.h>

#include "dr_list.h"
#include "colors.h"
#include "protocol.h"
#include "error_helper.h"
#include "file_db.h"

#define DEBUG TRUE

#define PORT_DELIMITER 1024 // just to preserve reserved port
#define HASH_LENGTH 32 // md5 hash length
#define SECRET_KEY "secret"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 3000

#define MAX_LEN_UNAME 16
#define MAX_LEN_PWD 16

void clear_screen();

char **get_file_name(char *file_path);

#endif // GRID_FTP_UTILS_H
