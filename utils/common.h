//
// Created by Andrea Lacava on 18/09/19.
//

#ifndef GRID_FTP_UTILS_H
#define GRID_FTP_UTILS_H

#include "colors.h"
#include "protocol.h"
#include "error_helper.h"
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
#include <pthread.h>

#define DEBUG TRUE


void clear_screen() {
    printf("%s\033[1;1H\033[2J\n", KNRM);
    printf(">> ");
    ERROR_HELPER(fflush(stdout), "Errore fflush", TRUE);
}

#endif // GRID_FTP_UTILS_H
