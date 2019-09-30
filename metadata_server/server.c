//
// Created by Andrea Lacava on 18/09/19.
//

#include "server.h"

sig_atomic_t server_on = TRUE;

typedef struct thread_args {
    int client_desc;
    DR_List *list;
    Grid_File_DB *file_db;
} thread_args;

void close_server() {
    server_on = FALSE;
    printf("Closing server! Bye bye");
    exit(EXIT_SUCCESS);
}


void send_command_to_dr(DR_Node *node, char *command, char *cmd_args) {
    int ret;
    struct sockaddr_in repo_addr = {};

    int sock_d = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(sock_d, "Error in socket creation", TRUE);

    repo_addr.sin_addr.s_addr = inet_addr(node->ip);
    repo_addr.sin_family = AF_INET;
    repo_addr.sin_port = htons(node->port);

    ret = connect(sock_d, (struct sockaddr *) &repo_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        fprintf(stderr, "DR_Node %d is offline probably.", node->id);
        return;
    }

    char buf[BUFSIZ];
    craft_request_header(buf, command);
    strncat(buf, cmd_args, strlen(cmd_args));
    strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

    send_message(sock_d, buf, strlen(buf));
}


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
                "%sCan't open user database at \"%s\". Wrong path?%s\n", KRED,
                USERLIST_FILE_PATH, KNRM);
        exit(EXIT_FAILURE);
    }

    while (fgets(data_info, sizeof(data_info), fp) != NULL && strlen(data_info) > 1) {
        username = strtok(data_info, CNFG_FILE_DELIMITER);
        password = strtok(NULL, CNFG_FILE_DELIMITER);
        if (strncmp(username, buf, strlen(username)) == 0 &&
            strncmp(password, buf + strlen(username) + strlen(COMMAND_DELIMITER), strlen(password)) == 0) {
            return TRUE;
        }
    }

    fclose(fp);
    return FALSE;
}

// dr_id,start_block,final_block
char *data_block_division(Grid_File_DB *file_db, DR_List *list, char *name, unsigned long size) {
    char *result = (char *) malloc(sizeof(char) * BUFSIZ);
    unsigned long block_size = size / list->size;
    unsigned long remaining = size % list->size;

    char block_name[FILENAME_MAX];
    unsigned long start_block = 0;
    unsigned long end_block = block_size;
    DR_Node *node = list->node;
    Block_File *head = NULL;
    Block_File *block_new;

    for (int i = 0; i < list->size; i++, node = node->next) {
        block_new = new_block(block_name, node->id, start_block, end_block);
        head = append_block(head, block_new);
        block_to_string(result, block_new);

        start_block = end_block + 1;
        if (i + 1 == list->size) {
            end_block = end_block + block_size + remaining;
        } else {
            end_block += block_size;
            strncat(result, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
        }
    }

    add_file(file_db, name, size, head);

    return result;
}

void *client_handling(void *args) {
    thread_args *t_args = (thread_args *) args;
    DR_List *list = t_args->list;
    Grid_File_DB *file_db = t_args->file_db;
    int client_desc = t_args->client_desc;
    char buf[BUFSIZ];
    sig_atomic_t client_on = TRUE;

    while (server_on && client_on) {
        recv_message(client_desc, buf);
        if (strncmp(buf, LS_CMD, strlen(LS_CMD)) == 0) {

            craft_ack_response_header(buf);
            file_db_to_string(file_db, buf);
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, AUTH_CMD, strlen(AUTH_CMD)) == 0) {

            if (client_authentication(buf + strlen(AUTH_CMD) + strlen(COMMAND_DELIMITER))) {

                craft_ack_response_header(buf);
                char *key = crypt(SECRET_CLIENT, SALT_SECRET);
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
            long unsigned file_size = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);
            char *division_list = data_block_division(file_db, list, file_name, file_size);
            craft_ack_response_header(buf);
            strncat(buf, division_list, strlen(division_list));
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, GET_CMD, strlen(GET_CMD)) == 0) {

            strtok(buf, COMMAND_DELIMITER); // we just ignore the GET_CMD header
            char *file_name = strtok(NULL, COMMAND_DELIMITER);
            char *file_str = file_to_string(get_file(file_db, file_name));

            craft_ack_response_header(buf);
            strncat(buf, file_str, strlen(file_str));
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, REMOVE_CMD, strlen(REMOVE_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER); // we just ignore the REMOVE_CMD header
            char *file_name = strtok(NULL, COMMAND_DELIMITER);

            if (remove_file(file_db, file_name)) {
                craft_ack_response(buf);

                char command_args[BUFSIZ];
                char *key = get_key(SECRET_SERVER);
                strncat(command_args, key, strlen(key));
                strncat(command_args, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
                strncat(command_args, file_name, strlen(file_name));

                for (DR_Node *node = list->node; node; node = node->next)
                    send_command_to_dr(node, REMOVE_CMD, command_args);
            } else
                craft_nack_response(buf);

            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, QUIT_CMD, strlen(QUIT_CMD)) == 0) {
            client_on = FALSE;
        } else { // Unrecognized command
            craft_nack_response(buf);
            send_message(client_desc, buf, strlen(buf));
        }
    }

    close(client_desc);
    pthread_exit(NULL);
}


/**
 * Main routine responsible of client requests handling
 */
void server_routine(DR_List *list, Grid_File_DB *file_db) {
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
        t_args->file_db = file_db;
        ret = pthread_create(&thread, NULL, client_handling, (void *) t_args);
        PTHREAD_ERROR_HELPER(ret, "Error on pthread creation", TRUE);
        ret = pthread_detach(thread);
        PTHREAD_ERROR_HELPER(ret, "Error on thread detaching", TRUE);
    }
}


int start_connection_with_dr(DR_Node *pNode, DR_List *list, Grid_File_DB *file_database) {
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
        fprintf(stderr, "DR_Node %d is offline probably.", pNode->id);
        delete_node(list, pNode->id);
        return ret;
    }

    char buf[BUFSIZ];
    craft_request_header(buf, DR_UPDATE_MAP_CMD);
    char *key = get_key(SECRET_SERVER);
    strncat(buf, key, strlen(key));
    send_message(sock_d, buf, strlen(buf));
    recv_message(sock_d, buf);
    if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
        update_file_db_from_string(file_database, buf + strlen(OK_RESPONSE));
    } else if (strncmp(buf, NOK_RESPONSE, strlen(NOK_RESPONSE)) == 0) {
        fprintf(stderr, "DR_Node %d refused the command \"%s\"", pNode->id, DR_UPDATE_MAP_CMD);
    } else {
        fprintf(stderr, "Unexpected response from node %d : %s", pNode->id, buf);
    }

    return sock_d;
}

int main(int argc, char const *argv[]) {

    struct sigaction sigint_action;
    sigset_t block_mask;

    sigfillset(&block_mask);
    sigint_action.sa_handler = close_server;
    sigint_action.sa_mask = block_mask;
    sigint_action.sa_flags = 0;
    int ret = sigaction(SIGINT, &sigint_action, NULL);
    ERROR_HELPER(ret, "Error on arming SIGINT: ", TRUE);

    signal(SIGPIPE, SIG_IGN);

    printf("Start reading configuration file...\n");
    DR_List *list = get_data_repositories_info();
    Grid_File_DB *file_db = init_db(FALSE, SERVER_ID);
    for (DR_Node *node = list->node; node; node = node->next)
        start_connection_with_dr(node, list, file_db);

    server_routine(list, file_db);

    return 0;
}

