//
// Created by Andrea Lacava on 18/09/19.
//

#ifndef GRID_FTP_UTILS_H
#define GRID_FTP_UTILS_H

#include "colors.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#define DEBUG TRUE

#define GENERIC_ERROR_HELPER(cond, errCode, msg)                               \
  do {                                                                         \
    if (cond) {                                                                \
      fprintf(stderr, "%s%s: %s\n", KRED, msg, strerror(errCode));             \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define ERROR_HELPER(ret, msg) GENERIC_ERROR_HELPER((ret < 0), errno, msg)
#define PTHREAD_ERROR_HELPER(ret, msg)                                         \
  GENERIC_ERROR_HELPER((ret != 0), ret, msg)

void clear_screen() {
  printf("%s\033[1;1H\033[2J\n", KNRM);
  printf(">> ");
  ERROR_HELPER(fflush(stdout), "Errore fflush");
}

#endif // GRID_FTP_UTILS_H
