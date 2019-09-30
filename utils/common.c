//
// Created by Andrea Lacava on 25/09/19.
//


#include "common.h"


void clear_screen() {
    printf("%s\033[1;1H\033[2J\n", KNRM);
    ERROR_HELPER(fflush(stdout), "Error on fflush", TRUE);
}


char **get_file_name(char *file_path) {

    // we use a low level operation for file instead of fopen()
    // since sendfile wants a descriptor and not a pointer to file (FILE*).
    struct stat file_stat;
    int fd, ret;
    if ((fd = open(file_path, O_RDONLY)) < 0) {
        fprintf(stderr, "Can't open file at \"%s\". Wrong path?\n", file_path);
        return NULL;
    }

    printf("File opened.\n");

    ret = fstat(fd, &file_stat);
    ERROR_HELPER(ret, "Error on retrieving file statistics", FALSE);
    if (ret < 0)
        return NULL;
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

int check_key(char *key, char *secret) {
    char *enc_key = crypt(key, SALT_SECRET);
    char *enc_secret = crypt(secret, SALT_SECRET);

    return strncmp(enc_key, enc_secret, strlen(enc_key)) == 0;
}

char *get_key(char *secret) {
    return crypt(secret, SALT_SECRET);
}

