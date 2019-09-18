//
// Created by Andrea Lacava on 18/09/19.
//

#include "server.h"

DR_List *get_data_repositories_info() {
    FILE *fp;
    char data_info[BUFSIZ];
    if ((fp = fopen(DATAREP_FILE_PATH, "r")) == NULL) {
        fprintf(stderr,
                "%sCan't open configuration file at \"%s\". Wrong path?%s\n", KRED,
                DATAREP_FILE_PATH, KNRM);
        exit(EXIT_FAILURE);
    }
    DR_List *list = new_list();
    int id_rep;
    char *ip_rep;

    while (fgets(data_info, sizeof(data_info), fp) != NULL &&
           strlen(data_info) > 1) {
        id_rep = (int) strtol(strtok(data_info, CNFG_DELIM), NULL, 10);
        ip_rep = strtok(NULL, CNFG_DELIM);
        ip_rep[strlen(ip_rep) - 1] = 0;
        if (DEBUG)
            printf("Repository number %d, ip: %s\n", id_rep, ip_rep);
        append_to_list(list, new_node(id_rep, ip_rep));
    }

    fclose(fp);
    return list;
}


int main(int argc, char const *argv[]) {
    printf("Start reading configuration file...\n");
    DR_List *list = get_data_repositories_info();
    printf("List size: %d\n", list->size);
    print_list(list);
    return 0;
}
