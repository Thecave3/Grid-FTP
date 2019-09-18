//
// Created by Andrea Lacava on 18/09/19.
//

#include "server.h"

/**
 * 
 */
char **get_data_repositories_info() {
    FILE *fp;
    char data_info[BUFSIZ];
    if ((fp = fopen(DATAREP_FILE_PATH, "r")) == NULL) {
        fprintf(stderr,
                "%sCan't open configuration file at \"%s\". Wrong path?%s\n", KRED,
                DATAREP_FILE_PATH, KNRM);
        exit(EXIT_FAILURE);
    }

    int nr_rep;
    char *ip_rep;
    while (fgets(data_info, sizeof(data_info), fp) != NULL &&
           strlen(data_info) > 1) {
        nr_rep = strtol(strtok(data_info, CNFG_DELIM), NULL, 10);
        ip_rep = strtok(NULL, CNFG_DELIM);
        ip_rep[strlen(ip_rep) - 1] = 0;
        if (DEBUG)
            printf("repository numero %d, ip: %s\n", nr_rep, ip_rep);
    }

    fclose(fp);
    return NULL;
}

int main(/*int argc, char const *argv[]*/) {
    printf("Start reading configuration file...\n");
    get_data_repositories_info();
    return 0;
}
