//
// Created by Andrea Lacava on 20/09/19.
//

#ifndef GRID_FTP_ERROR_HELPER_H
#define GRID_FTP_ERROR_HELPER_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "colors.h"

#define TRUE 1
#define FALSE 0

/**
 * Just some useful stuff to handle errors better
 */
 
#define GENERIC_ERROR_HELPER(cond, errCode, msg, isFatal)                       \
  do {                                                                         \
    if (cond) {                                                                \
      fprintf(stderr, "%s%s: %s%s\n", KRED, msg, strerror(errCode),KNRM);             \
      if(isFatal)                                                              \
        exit(EXIT_FAILURE);                                                     \
    }                                                                          \
  } while (0)

#define ERROR_HELPER(ret, msg, isFatal) GENERIC_ERROR_HELPER((ret < 0), errno, msg,isFatal)
#define PTHREAD_ERROR_HELPER(ret, msg, isFatal)                                         \
  GENERIC_ERROR_HELPER((ret != 0), ret, msg,isFatal)


#endif //GRID_FTP_ERROR_HELPER_H
