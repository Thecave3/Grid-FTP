//
// Created by Andrea Lacava on 18/09/19.
//

#ifndef GRID_FTP_UTILS_H
#define GRID_FTP_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <semaphore.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <crypt.h>
#include <libgen.h>

#include "dr_list.h"
#include "colors.h"
#include "protocol.h"
#include "error_helper.h"
#include "file_db.h"

#define DEBUG TRUE

#define PORT_DELIMITER 1024 // just to preserve reserved port

#define SECRET_DR "dr_secret"
#define SECRET_SERVER "server_secret"
#define SECRET_CLIENT "client_secret"
#define SALT_SECRET "Az"

#define FILE_BLOCK_SEPARATOR "_"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 3000
#define SERVER_ID 0

#define MAX_LEN_UNAME 16
#define MAX_LEN_PWD 16

void clear_screen();

char **get_file_name(char *file_path);

int check_key(char *key, char *secret);

char* get_key(char* secret);


#endif // GRID_FTP_UTILS_H
