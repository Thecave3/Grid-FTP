//
// Created by Andrea Lacava on 18/09/19.
//

#include "server.h"

sig_atomic_t server_on = 1;

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


int start_connection_with_dr(Node *pNode) {
    printf("Starting connection with node %d at %s:%u\n", pNode->id, pNode->ip, pNode->port);
    int ret, sock;
    struct sockaddr_in repo_addr = {};

    // Socket creation
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(socket_desc, "Error in socket creation", TRUE);

    // Set up connection parameters
    repo_addr.sin_addr.s_addr = inet_addr(pNode->ip);
    repo_addr.sin_family = AF_INET;
    repo_addr.sin_port = htons(pNode->port);

    // Connection
    ret = connect(socket_desc, (struct sockaddr *) &repo_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        fprintf(stderr, "Node %d is offline probably.", pNode->id);
        return ret;
    }
    sock = ret;

    // TODO Authentication phase
    // TODO get updated allocation map from each repository
    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ); // just clean the buffer and make sure no strange things is inside
    strncpy(buf, DR_UPDATE_MAP, strlen(DR_UPDATE_MAP));
    send_message(sock, buf);
    recv_message(sock, buf);
    printf("%s", buf);

    return sock;
}

int main(int argc, char const *argv[]) {

    //TODO handling SIGINT + SIGPIPE

    printf("Start reading configuration file...\n");
    DR_List *list = get_data_repositories_info();
//    print_list(list);
    int dr_socks[list->size];
    int i = 0;

    for (Node *node = list->node; node; node = node->next, i++)
        dr_socks[i] = start_connection_with_dr(node);

//    for (int j = 0; j < list->size; ++j) {
//        printf("%d", dr_socks[j]);
//    }

    // server routine

//    pid_t pid = fork();
//    ERROR_HELPER(pid, "Error in the fork of the main process", TRUE);
//    if (pid == 0) {
//        printf("I am child\n\n");
//        return 0;
//    } else {
//        printf("I am parent\n\n");
//        server_routine();
//        return 0;
//    }
}

/**
 * Main routine responsible of client requests handling
 */
void server_routine() {
    int server_sock = server_init(SERVER_PORT);
    int client_sock;
    struct sockaddr client_addr;
    int client_addr_len = sizeof(client_addr);

    printf("Begin server routine\n");

    while (server_on) {

        memset(&client_addr, 0, sizeof(client_addr));
        client_sock = accept(server_sock, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
        if (client_sock < 0) continue;

        printf("client connected");

    }
}
