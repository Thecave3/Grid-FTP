#include "client.h"

typedef struct thread_args {
    DR_List *list;
    u_int8_t id_dr;
    char *filename;
    char *key;
    unsigned long start, end;
} thread_args;

sig_atomic_t keep_alive = TRUE;

#define CRITICAL_SECTION_SIZE 1

sem_t sem;

/**
 * Prints a list for program usage
 */
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

/**
 * just a stub to close the client
 */
void close_client() {
    keep_alive = FALSE;
}

/**
 *
 * Sends the QUIT_CMD to server
 *
 * @param client_desc endpoint to alert
 */
void quit_command(int client_desc) {
    size_t buffer_size = BUFSIZ;
    char buf[buffer_size];
    craft_request(buf, QUIT_CMD, buffer_size);
    send_message(client_desc, buf, strlen(buf));
    close(client_desc);
    close_client();
}

/**
 *  Initialize a client
 *
 * @param address target
 * @param port target
 * @return descriptor of th socket connected
 */
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
    size_t buffer_size = BUFSIZ;
    char buf[buffer_size];
    int ret;

    printf("Username: ");
    ret = fgets(username, sizeof(username), stdin) != (char *) username;
    ERROR_HELPER(ret, "Error on input read", TRUE);
    username[strlen(username) - 1] = 0;

    printf("Password: ");
    ret = fgets(password, sizeof(password), stdin) != (char *) password;
    ERROR_HELPER(ret, "Error on input read", TRUE);
    password[strlen(password) - 1] = 0;

    craft_request_header(buf, AUTH_CMD, buffer_size);
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
        return strdup(key);
    } else {
        return NULL;
    }
}

/**
 *
 * Split the file in more parts before sending it.
 *
 * @param filename file to split
 * @param block_name name of the block to create
 * @param start initial offeset
 * @param end final offset
 */
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

/**
 *  Thread routine to send the file to the server
 *
 * @param args see t_args
 * @return
 */
void *send_file_to_dr(void *args) {
    thread_args *t_args = (thread_args *) args;
    DR_List *list = t_args->list;
    char *key = t_args->key;
    u_int8_t id_dr = t_args->id_dr;
    char *filename = t_args->filename;
    unsigned long start = t_args->start;
    unsigned long end = t_args->end;

    size_t buffer_size = BUFSIZ;

    char buf[buffer_size];
    char block_name[buffer_size];
    char start_str[buffer_size];
    char end_str[buffer_size];

    memset(block_name, 0, strlen(block_name));
    memset(start_str, 0, strlen(start_str));
    memset(end_str, 0, strlen(end_str));

    snprintf(block_name, BUFSIZ, "%s%s%hhu", filename, FILE_BLOCK_SEPARATOR, id_dr);
    snprintf(start_str, BUFSIZ, "%lu", start);
    snprintf(end_str, BUFSIZ, "%lu", end);

    sem_wait(&sem);
    split_file(filename, block_name, start, end);
    sem_post(&sem);

    DR_Node *node = get_node(list, id_dr);
    int dr_sock = client_init(node->ip, node->port);

    craft_request_header(buf, PUT_CMD, buffer_size);
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

/**
 * A custom implementation of the ls command of bash
 *
 * This function exploit exactly the "ls " command without the "." and ".." folders
 *
 */
void ls_command() {
    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ);

    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] != '.') { // ignore the hidden file and the back
                printf("- %s\n", dir->d_name);
            }
        }
        closedir(d);
    }
}

/**
 *
 * Download function from data repository
 *
 * @param list DR_List having all the data repositories
 * @param key secret key taken from server
 * @param filename name of the file to retrieve
 * @param dr_id id of the repository where the file is located
 * @param start offset
 * @param end offset
 */
void get_file_from_dr(DR_List *list, char *key, char *filename, char *dr_id, char *start, char *end) {
    size_t buffer_size = BUFSIZ;
    char buf[buffer_size];
    craft_request_header(buf, GET_CMD, buffer_size);
    strncat(buf, key, strlen(key));
    strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    strncat(buf, filename, strlen(filename));

    DR_Node *node = get_node(list, strtol(dr_id, NULL, 10));
    int dr_sock = client_init(node->ip, node->port);

    send_message(dr_sock, buf, strlen(buf));

    recv_message(dr_sock, buf);

    if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
        long unsigned size = strtol(end, NULL, 10) - strtol(start, NULL, 10);
        recv_file(dr_sock, filename, size);
        printf("File received!\n");
    } else {
        printf("%sData repository refused%s!", KRED, KNRM);
    }
}

/**
 *
 * Parse user input and sends commands to server
 *
 * @param client_desc descriptor of the client connected to the server
 * @param key secret key taken from server
 * @param list of known dr in the network
 */
void client_routine(int client_desc, char *key, DR_List *list) {
    size_t buffer_size = BUFSIZ;
    char buf[buffer_size];
    int ret;

    while (keep_alive) {
        printf("%sType a command: %s", KBLU, KNRM);
        ret = fgets(buf, buffer_size, stdin) != (char *) buf;
        ERROR_HELPER(ret, "Error on input read", TRUE);

        if (strncmp(buf, CLEAR, strlen(CLEAR)) == 0) {
            clear_screen();
        } else if (strncmp(buf, HELP, strlen(HELP)) == 0) {
            commands_available();
        } else if (strncmp(buf, LS, strlen(LS)) == 0) {
            craft_request(buf, LS_CMD, buffer_size);
            send_message(client_desc, buf, strlen(buf));

            recv_message(client_desc, buf);
            if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
                printf("File list:\n");
                printf("%s", buf + strlen(OK_RESPONSE) + strlen(COMMAND_DELIMITER));
            } else {
                printf("%sProblem in communication!%s", KRED, KNRM);
            }
        } else if (strncmp(buf, PUT, strlen(PUT)) == 0) {
            char file_path[FILENAME_MAX];
            printf("%sThis directory:\n", KBLU);
            ls_command();
            printf("%s", KNRM);
            printf("%sPath of the file: %s", KBLU, KNRM);

            ret = fgets(file_path, sizeof(file_path), stdin) != (char *) file_path;
            ERROR_HELPER(ret, "Error on input read", TRUE);
            file_path[strlen(file_path) - 1] = 0;
            char **file_info = get_file_name(file_path);
            if (!file_info)
                continue;
            char *file_name = file_info[0];
            char *file_size = file_info[1];

            craft_request_header(buf, PUT_CMD, buffer_size);
            strncat(buf, file_name, strlen(file_name));
            strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
            strncat(buf, file_size, strlen(file_size));
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

            send_message(client_desc, buf, strlen(buf));

            recv_message(client_desc, buf);

            // ack,B,user.db_1,1,0,30
            if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
                // parse list of block and dr
                // dr_id,start_block,final_block
                sem_init(&sem, 0, CRITICAL_SECTION_SIZE);

                char *part = strtok(buf, COMMAND_DELIMITER);
                char *dr_id_str, *start_str, *end_str;
                u_int8_t dr_id;
                long unsigned start, final;
                pthread_t thread[100];
                thread_args *t_args;
                int index = 0;
                while (part != NULL) {
                    strtok(NULL, COMMAND_DELIMITER); // BLOCK_DELIMITER
                    strtok(NULL, COMMAND_DELIMITER); // block name
                    dr_id_str = strtok(NULL, COMMAND_DELIMITER);
                    start_str = strtok(NULL, COMMAND_DELIMITER);
                    end_str = strtok(NULL, COMMAND_DELIMITER);

                    if (dr_id_str != NULL && end_str != NULL && start_str != NULL) {
                        dr_id = strtol(dr_id_str, NULL, 10);
                        start = strtol(start_str, NULL, 10);
                        final = strtol(end_str, NULL, 10);

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
                        index++;
                    }
                    part = strtok(NULL, COMMAND_DELIMITER);
                }

                for (index = 0; index < 100; index++) {
                    pthread_join(thread[index], NULL);
                }

                sem_destroy(&sem);

            } else {
                printf("%sServer has refused command \"%s\"%s\n", KRED, PUT_CMD, KNRM);
                printf("%sSomething strange is going on, I go out\n%s", KRED, KNRM);
                quit_command(client_desc);
            }

        } else if (strncmp(buf, GET, strlen(GET)) == 0) {
            char filename[FILENAME_MAX];
            printf("%sFilename: %s", KBLU, KNRM);
            ret = fgets(filename, sizeof(filename), stdin) != (char *) filename;
            ERROR_HELPER(ret, "Error on input read", TRUE);

            craft_request_header(buf, GET_CMD, buffer_size);
            strncat(buf, filename, strlen(filename));
            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

            send_message(client_desc, buf, strlen(buf));

            recv_message(client_desc, buf);

            if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
                printf("File info retrieved!\n");
                strtok(buf, COMMAND_DELIMITER); // OK RESPONSE
                char *delimiter = strtok(NULL, COMMAND_DELIMITER); // Block delimiter
                char *block_name, *dr_id_str, *start_str, *end_str;
                while (delimiter != NULL && strncmp(delimiter, BLOCK_DELIMITER, strlen(BLOCK_DELIMITER)) == 0) {
                    block_name = strtok(NULL, COMMAND_DELIMITER);// block name
                    dr_id_str = strtok(NULL, COMMAND_DELIMITER); // dr id
                    start_str = strtok(NULL, COMMAND_DELIMITER); // start
                    end_str = strtok(NULL, COMMAND_DELIMITER); // end

                    get_file_from_dr(list, key, block_name, dr_id_str, start_str, end_str);

                    delimiter = strtok(NULL, COMMAND_DELIMITER);
                }
            } else {
                printf("File not found, server has answered with nack!\n");
            }

        } else if (strncmp(buf, REMOVE, strlen(REMOVE)) == 0) {
            char filename[FILENAME_MAX];
            printf("%sFilename: %s", KBLU, KNRM);
            ret = fgets(filename, sizeof(filename), stdin) != (char *) filename;
            ERROR_HELPER(ret, "Error on input read", TRUE);

            craft_request_header(buf, REMOVE_CMD, buffer_size);
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
    size_t buffer_size = BUFSIZ;
    char buf[buffer_size];

    craft_request(buf, GET_DR_CMD, buffer_size);

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

    printf("%sGrid FTP Client%s\n", KBLU, KNRM);

    client_routine(client_desc, key, list);

    return 0;
}