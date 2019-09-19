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
    u_int16_t port_rep;
    while (fgets(data_info, sizeof(data_info), fp) != NULL &&
           strlen(data_info) > 1) {
        id_rep = (int) strtol(strtok(data_info, CNFG_FILE_DEL), NULL, 10);
        ip_rep = strtok(NULL, CNFG_FILE_DEL);
        port_rep = strtol(strtok(NULL, CNFG_FILE_DEL), NULL, 10);
        append_to_list(list, new_node(id_rep, ip_rep, port_rep));
    }

    fclose(fp);
    return list;
}


void start_connection(Node *pNode) {
    printf("Starting connection with node %d at %s:%u\n", pNode->id, pNode->ip, pNode->port);
    int ret;
    struct sockaddr_in repo_addr = {};

    // Socket creation
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(socket_desc, "Error in socket creation");

    // Set up connection parameters
    repo_addr.sin_addr.s_addr = inet_addr(pNode->ip);
    repo_addr.sin_family = AF_INET;
    repo_addr.sin_port = htons(pNode->port);

    // Connection
    ret = connect(socket_desc, (struct sockaddr *) &repo_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Error in the data_repository connection");

}

int main(int argc, char const *argv[]) {

    //TODO handling SIGINT + SIGPIPE

    printf("Start reading configuration file...\n");
    DR_List *list = get_data_repositories_info();
    print_list(list);
    for (Node *node = list->node; node; node = node->next)
        start_connection(node);

    return 0;
}
