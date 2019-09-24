//
// Created by Andrea Lacava on 18/09/19.
//

#include "server.h"

sig_atomic_t server_on = 1;

typedef struct thread_args {
    int client_desc;
    DR_List *list;
} thread_args;

DR_List *get_data_repositories_info() {
    FILE *fp;
    char data_info[BUFSIZ];
    if (!(fp = fopen(DATAREP_FILE_PATH, "r"))) {
        fprintf(stderr,
                "%sCan't open configuration file at \"%s\". Wrong path?%s\n", KRED,
                DATAREP_FILE_PATH, KNRM);
        exit(EXIT_FAILURE);
    }
    DR_List *list = new_list();
    u_int8_t id_rep;
    char *ip_rep;
    u_int16_t port_rep;

    while (fgets(data_info, sizeof(data_info), fp) != NULL && strlen(data_info) > 1) {
        id_rep = (u_int8_t) strtol(strtok(data_info, CNFG_FILE_DELIMITER), NULL, 10);
        ip_rep = strtok(NULL, CNFG_FILE_DELIMITER);
        port_rep = strtol(strtok(NULL, CNFG_FILE_DELIMITER), NULL, 10);
        append_to_list(list, new_node(id_rep, ip_rep, port_rep));
    }

    fclose(fp);
    return list;
}

int client_authentication(char *buf) {
    char *username;
    char *password;
    FILE *fp;
    char data_info[BUFSIZ];
    if (!(fp = fopen(USERLIST_FILE_PATH, "r"))) {
        fprintf(stderr,
                "%sCan't open userdb at \"%s\". Wrong path?%s\n", KRED,
                USERLIST_FILE_PATH, KNRM);
        exit(EXIT_FAILURE);
    }

    while (fgets(data_info, sizeof(data_info), fp) != NULL && strlen(data_info) > 1) {
        username = strtok(data_info, CNFG_FILE_DELIMITER);
        password = strtok(NULL, CNFG_FILE_DELIMITER);
        // TODO debug
        if (strncmp(username, buf, strlen(username)) == 0 &&
            strncmp(password, buf + strlen(username), strlen(password)) == 0) {
            return TRUE;
        }
    }

    fclose(fp);
    return FALSE;
}

void *client_handling(void *args) {
    thread_args *t_args = (thread_args *) args;
    DR_List *list = t_args->list;
    int client_desc = t_args->client_desc;
    char buf[BUFSIZ];
    while (server_on) {
        recv_message(client_desc, buf);
        if (strncmp(buf, AUTH_CMD, strlen(AUTH_CMD)) == 0) {
            if (client_authentication(buf + strlen(AUTH_CMD))) {
                char *key = (char *) malloc(sizeof(char) * HASH_LENGTH);
                strncpy(key, "chiave", strlen("chiave"));
                craft_ack_response_header(buf);
                strncat(buf, key, strlen(key));
                strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
            } else {
                craft_nack_response(buf);
            }
            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, GET_DR_CMD, strlen(GET_DR_CMD)) == 0) {
            char *dr_list_string = list_to_string(list);
            craft_ack_response_header(buf);
            strncat(buf, dr_list_string, strlen(dr_list_string));
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
            send_message(client_desc, buf, strlen(buf));
        } else if (strncmp(buf, PUT_CMD, strlen(PUT_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER); // we just ignore the PUT_CMD header
            char *file_name = strtok(NULL, COMMAND_DELIMITER);
            // TODO check if this or COMMAND TERMINATOR
            long unsigned file_size = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);
            craft_ack_response(buf);
            send_message(client_desc, buf, strlen(buf));
            FILE *fp = recv_file(client_desc, file_name, file_size);
            // TODO divide file in blocks and end PUT_CMD

        } else if (strncmp(buf, GET_CMD, strlen(GET_CMD)) == 0) {

        } else if (strncmp(buf, REMOVE_CMD, strlen(REMOVE_CMD)) == 0) {

        } else { // Unrecognized command
            craft_nack_response(buf);
            send_message(client_desc, buf, strlen(buf));
        }
    }

    pthread_exit(NULL);
}


/**
 * Main routine responsible of client requests handling
 */
void server_routine(DR_List *list) {
    int server_sock = server_init(SERVER_PORT);
    struct sockaddr client_addr;
    int client_addr_len = sizeof(client_addr);
    int client_desc, ret;
    pthread_t thread;
    thread_args *t_args;

    printf("Begin server routine\n");

    while (server_on) {
        memset(&client_addr, 0, sizeof(client_addr));
        client_desc = accept(server_sock, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
        if (client_desc < 0) continue;
        t_args = (thread_args *) malloc(sizeof(thread_args));
        t_args->list = list;
        t_args->client_desc = client_desc;
        ret = pthread_create(&thread, NULL, client_handling, (void *) t_args);
        PTHREAD_ERROR_HELPER(ret, "Error on pthread creation", TRUE);
        ret = pthread_detach(thread);
        PTHREAD_ERROR_HELPER(ret, "Error on thread detaching", TRUE);
    }
}


int start_connection_with_dr(Node *pNode) {
    printf("Starting connection with node %u at %s : %u\n", pNode->id, pNode->ip, pNode->port);
    int ret;
    struct sockaddr_in repo_addr = {};

    int sock_d = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(sock_d, "Error in socket creation", TRUE);

    repo_addr.sin_addr.s_addr = inet_addr(pNode->ip);
    repo_addr.sin_family = AF_INET;
    repo_addr.sin_port = htons(pNode->port);

    ret = connect(sock_d, (struct sockaddr *) &repo_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        fprintf(stderr, "Node %d is offline probably.", pNode->id);
        return ret;
    }

    // TODO Authentication phase
    // TODO get updated allocation map from each repository
    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ); // just clean the buffer and make sure no strange things are inside
    strncpy(buf, DR_UPDATE_MAP_CMD, strlen(DR_UPDATE_MAP_CMD));
    send_message(sock_d, buf, strlen(DR_UPDATE_MAP_CMD));
    recv_message(sock_d, buf);

    if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
        printf("He said yes");
    } else if (strncmp(buf, NOK_RESPONSE, strlen(NOK_RESPONSE)) == 0) {
        fprintf(stderr, "Node %d refused the command \"%s\"", pNode->id, DR_UPDATE_MAP_CMD);
    } else {
        fprintf(stderr, "Unexpected response from node %d : %s", pNode->id, buf);
    }

    return sock_d;
}

int main(int argc, char const *argv[]) {

    //TODO handling SIGINT + SIGPIPE

    printf("Start reading configuration file...\n");
    DR_List *list = get_data_repositories_info();
    for (Node *node = list->node; node; node = node->next)
        start_connection_with_dr(node);

    server_routine(list);

    return 0;
}

