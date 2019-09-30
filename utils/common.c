//
// Created by Andrea Lacava on 25/09/19.
//


#include "common.h"

/**
 * Clear screen in a quick way and restore KNRM color in every case
 *
 */
void clear_screen() {
    printf("%s\033[1;1H\033[2J\n", KNRM);
    ERROR_HELPER(fflush(stdout), "Error on fflush", TRUE);
}

/**
 * Get information of a file in order to use sendfile
 *
 * @param file_path path of the file to open
 * @return [file_name,file_size]
 */
char **get_file_name(char *file_path) {

    // we use a low level operation for file instead of fopen()
    // since sendfile wants a descriptor and not a pointer to file (FILE*).
    struct stat file_stat;
    int fd, ret;
    if ((fd = open(file_path, O_RDONLY)) < 0) {
        fprintf(stderr, "Can't open file at \"%s\". Wrong path?\n", file_path);
        return NULL;
    }

    printf("File\"%s\" opened.\n", file_path);

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

/**
 * Check if the key provided correspond to the one of the secret. Since the key is shared and it is the same for all the user is not a so good protection.
 * In a more elaborate version the key would be a time token.
 *
 * @param key to be inspected
 * @param secret see common.h to know what are the secrets
 * @return TRUE if everything's good, FALSE elsewhere
 */
int check_key(char *key, char *secret) {
    char *enc_key = get_key(key);
    char *enc_secret = get_key(secret);

    return strncmp(enc_key, enc_secret, strlen(enc_key)) == 0;
}

/**
 * Just a mask used in order to make any changes of the encryption algorithm more easier to implement.
 *
 * @param secret key to be encrypted
 * @return the secret encrypted
 */
char *get_key(char *secret) {
    return crypt(secret, SALT_SECRET);
}

