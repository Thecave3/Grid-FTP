#include "client.h"

typedef struct thread_args {
    DR_List *list;
    char *key;
    u_int8_t id_dr;
    char *filename;
    unsigned long start, end;
} thread_args;

sig_atomic_t keep_alive = TRUE;

#define CRITICAL_SECTION_SIZE 1

sem_t sem;

void commands_available() {
    printf("Commands available:\n");
    printf("- \"%s\": clear the screen.\n", CLEAR);
    printf("- \"%s\": print this list.\n", HELP);
    printf("- \"%s\": list all the file.\n", LS);
    printf("- \"%s\": put new file in data repositories.\n", PUT);
    printf("- \"%s\": get file from data repositories.\n", GET);
    printf("- \"%s\": remove file from data repositories.\n", REMOVE);
    printf("- \"%s\": close the program.\n", EXIT_CLIENT);
}


void close_client() {
    keep_alive = FALSE;
}


void quit_command(int client_desc) {
    char buf[BUFSIZ];
    craft_request(buf, QUIT_CMD);
    send_message(client_desc, buf, strlen(buf));
    close(client_desc);
    close_client();
}

int client_init(char *address, u_int16_t port) {
    int ret;
    struct sockaddr_in repo_addr = {};

    int sock_d = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(sock_d, "Error in socket creation", TRUE);

    repo_addr.sin_addr.s_addr = inet_addr(address);
    repo_addr.sin_family = AF_INET;
    repo_addr.sin_port = htons(port);
    ret = connect(sock_d, (struct sockaddr *) &repo_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Error in connection", TRUE);

    return sock_d;
}

/**
 * Gets username and password from the user and sends an AUTH_CMD request to the server
 *
 * @param client_desc descriptor of the connection with the server
 * @return a string representing the key for communication with data repositories or NULL if credentials are not good
 */
char *authentication(int client_desc) {
    char username[MAX_LEN_UNAME];
    char password[MAX_LEN_PWD];
    char buf[BUFSIZ];
    int ret;

    printf("Username: ");
    ret = fgets(username, sizeof(username), stdin) != (char *) username;
    ERROR_HELPER(ret, "Error on input read", TRUE);
    username[strlen(username) - 1] = 0;

    printf("Password: ");
    ret = fgets(password, sizeof(password), stdin) != (char *) password;
    ERROR_HELPER(ret, "Error on input read", TRUE);
    password[strlen(password) - 1] = 0;

    craft_request_header(buf, AUTH_CMD);
    strncat(buf, username, strlen(username));
    strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    strncat(buf, password, strlen(password));
    strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

    send_message(client_desc, buf, strlen(buf));

    recv_message(client_desc, buf);

    if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
        char *key = strtok(buf + strlen(OK_RESPONSE), COMMAND_DELIMITER);
        int i = 0;
        while (key[i] != '\n')
            i++;

        key[i] = 0;
        return key;
    } else {
        return NULL;
    }
}


void split_file(char *filename, char *block_name, unsigned long start, unsigned long end) {
    FILE *src, *dest;
    int ch;

    src = fopen(filename, "rb");
    dest = fopen(block_name, "wb");
    if (src == NULL || dest == NULL) {
        ERROR_HELPER(-1, "Error on file open", TRUE);
    }

    fseek(src, (long) start, SEEK_SET);
    for (long unsigned i = start; i < end; i++) {
        ch = getc(src);
        if (ch == EOF)
            break;
        putc(ch, dest);
    }

    fclose(src);
    fclose(dest);
}


void *send_file_to_dr(void *args) {
    thread_args *t_args = (thread_args *) args;
    DR_List *list = t_args->list;
    char *key = t_args->key;
    u_int8_t id_dr = t_args->id_dr;
    char *filename = t_args->filename;
    unsigned long start = t_args->start;
    unsigned long end = t_args->end;

    char buf[BUFSIZ];
    char block_name[BUFSIZ];
    char start_str[BUFSIZ];
    char end_str[BUFSIZ];

    memset(block_name, 0, strlen(block_name));
    memset(start_str, 0, strlen(start_str));
    memset(end_str, 0, strlen(end_str));

    snprintf(block_name, sizeof(block_name), "%s%s%hhu", filename, FILE_BLOCK_SEPARATOR, id_dr);
    snprintf(block_name, sizeof(start_str), "%lu", start);
    snprintf(block_name, sizeof(end_str), "%lu", end);

    sem_wait(&sem);
    split_file(filename, block_name, start, end);
    sem_post(&sem);

    DR_Node *node = get_node(list, id_dr);
    int dr_sock = client_init(node->ip, node->port);

    craft_request_header(buf, PUT_CMD);
    strncat(buf, key, strlen(key));
    strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    strncat(buf, block_name, strlen(block_name));
    strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    strncat(buf, start_str, strlen(start_str));
    strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    strncat(buf, end_str, strlen(end_str));
    strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
    send_message(dr_sock, buf, strlen(buf));

    recv_message(dr_sock, buf);

    if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
        send_file(dr_sock, block_name, end - start);
    } else {
        printf("%sData repository #%hhu nacked!%s", KRED, id_dr, KNRM);
    }

    close(dr_sock);
    pthread_exit(NULL);
}

void client_routine(int client_desc, char *key, DR_List *list) {
    char buf[BUFSIZ];
    int ret;

    while (keep_alive) {
        ret = fgets(buf, sizeof(buf), stdin) != (char *) buf;
        ERROR_HELPER(ret, "Error on input read", TRUE);

        if (strncmp(buf, CLEAR, strlen(CLEAR)) == 0) {
            clear_screen();
        } else if (strncmp(buf, HELP, strlen(HELP)) == 0) {
            commands_available();
        } else if (strncmp(buf, LS, strlen(LS)) == 0) {
            craft_request_header(buf, LS_CMD);
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

            send_message(client_desc, buf, strlen(buf));

            recv_message(client_desc, buf);
            if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
                // Parse and print list
                printf("%s", buf);
            } else {
                printf("%sProblem in communication!%s", KRED, KNRM);
            }
        } else if (strncmp(buf, PUT, strlen(PUT)) == 0) {
            char file_path[FILENAME_MAX];
            printf("Path of the file: ");
            ret = fgets(file_path, sizeof(file_path), stdin) != (char *) file_path;
            ERROR_HELPER(ret, "Error on input read", TRUE);
            file_path[strlen(file_path) - 1] = 0;
            char **file_info = get_file_name(file_path);
            if (!file_info)
                continue;
            char *file_name = file_info[0];
            char *file_size = file_info[1];

            craft_request_header(buf, PUT_CMD);
            strncat(buf, file_name, strlen(file_name));
            strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
            strncat(buf, file_size, strlen(file_size));
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

            send_message(client_desc, buf, strlen(buf));

            recv_message(client_desc, buf);

            if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
                // parse list of block and dr
                // dr_id,start_block,final_block
                sem_init(&sem, 0, CRITICAL_SECTION_SIZE);

                strtok(buf, COMMAND_DELIMITER); // ignore the OK response

                char *part = strtok(NULL, COMMAND_DELIMITER);
                u_int8_t dr_id;
                long unsigned start, final;
                pthread_t thread[100];
                thread_args *t_args;
                int index = 0;
                while (part != NULL && index < 100) {
                    dr_id = strtol(part, NULL, 10);
                    start = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);
                    final = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);

                    t_args = (thread_args *) malloc(sizeof(thread_args));
                    t_args->list = list;
                    t_args->key = key;
                    t_args->id_dr = dr_id;
                    t_args->filename = file_name;
                    t_args->start = start;
                    t_args->end = final;

                    ret = pthread_create(&thread[index], NULL, send_file_to_dr, (void *) t_args);
                    PTHREAD_ERROR_HELPER(ret, "Error on pthread creation", TRUE);
                    ret = pthread_detach(thread[index]);
                    PTHREAD_ERROR_HELPER(ret, "Error on thread detaching", TRUE);


                    part = strtok(NULL, COMMAND_DELIMITER);
                    index++;
                }

                for (index = 0; index < 100; index++) {
                    pthread_join(thread[index], NULL);
                }

                sem_destroy(&sem);

            } else {
                printf("%sServer has refused command \"%s\"%s", KRED, PUT_CMD, KNRM);
            }

        } else if (strncmp(buf, GET, strlen(GET)) == 0) {
            char filename[FILENAME_MAX];
            printf("%sFilename: %s", KBLU, KNRM);
            ret = fgets(filename, sizeof(filename), stdin) != (char *) filename;
            ERROR_HELPER(ret, "Error on input read", TRUE);

            craft_request_header(buf, GET_CMD);
            strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
            strncat(buf, filename, strlen(filename));
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
            send_message(client_desc, buf, strlen(buf));
            recv_message(client_desc, buf);
            if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
                printf("File info retrieved!\n");
                // Parse list and get from DR
                printf("%s", buf);
            } else {
                printf("File not found!\n");
            }

        } else if (strncmp(buf, REMOVE, strlen(REMOVE)) == 0) {
            char filename[FILENAME_MAX];
            printf("%sFilename: %s", KBLU, KNRM);
            ret = fgets(filename, sizeof(filename), stdin) != (char *) filename;
            ERROR_HELPER(ret, "Error on input read", TRUE);

            craft_request_header(buf, REMOVE_CMD);
            strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
            strncat(buf, filename, strlen(filename));
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
            send_message(client_desc, buf, strlen(buf));
            recv_message(client_desc, buf);
            if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
                printf("File deleted!\n");
            } else {
                printf("File not deleted!\n");
            }

        } else if (strncmp(buf, EXIT_CLIENT, strlen(EXIT_CLIENT)) == 0) {

            quit_command(client_desc);
            printf("Connection closed, bye bye!\n");
        } else {

            printf("%s", buf);
            printf("%sError unrecognized command\n%s", KRED, KNRM);
            commands_available();
        }

    }
}

/**
 * Make a GET_DR_CMD request to the server in order to retrieve the data repositories currently online
 *
 * @param client_desc descriptor of the connection with the server
 * @return DR_List* representing all data repositories active in the grid
 */
DR_List *get_data_repositories(int client_desc) {
    char buf[BUFSIZ];

    craft_request(buf, GET_DR_CMD);

    send_message(client_desc, buf, strlen(buf));

    recv_message(client_desc, buf);

    if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
        strtok(buf, COMMAND_DELIMITER);
        DR_List *list = new_list();

        u_int8_t id_rep;
        char *id_rep_str, *ip_rep, *port_rep_str;
        u_int16_t port_rep;

        while ((id_rep_str = strtok(NULL, COMMAND_DELIMITER)) != NULL) {
            id_rep = (u_int8_t) strtol(id_rep_str, NULL, 10);
            ip_rep = strtok(NULL, COMMAND_DELIMITER);
            port_rep_str = strtok(NULL, COMMAND_DELIMITER);
            port_rep = strtol(port_rep_str, NULL, 10);
            append_to_list(list, new_node(id_rep, ip_rep, port_rep));
        }

        return list;
    } else {
        return NULL;
    }
}

int main() {

    struct sigaction sigint_action;
    sigset_t block_mask;

    sigfillset(&block_mask);
    sigint_action.sa_handler = close_client;
    sigint_action.sa_mask = block_mask;
    sigint_action.sa_flags = 0;
    int ret = sigaction(SIGINT, &sigint_action, NULL);
    ERROR_HELPER(ret, "Error on arming SIGINT: ", TRUE);

    int client_desc = client_init(SERVER_ADDR, SERVER_PORT);

    char *key = authentication(client_desc);
    if (!key) {
        perror("Authentication failed!\nExiting...\n");
        quit_command(client_desc);
        return 0;
    }

    DR_List *list = get_data_repositories(client_desc);
    if (!list) {
        perror("Error in the creation of the list, exiting");
        quit_command(client_desc);
        return -1;
    }

    printf("%sGrid FTP Client, type a command:%s\n", KBLU, KNRM);

    client_routine(client_desc, key, list);

    return 0;
}