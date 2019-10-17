//
// Created by Andrea Lacava on 18/09/19.
//

#include "data_repository.h"

#define CRITICAL_SECTION_DB_SIZE 1
sem_t sem_db_file;

typedef struct thread_args {
    int client_desc;
    Grid_File_DB *file_db;
} thread_args;


sig_atomic_t dr_on = TRUE;

char *localpath;

/**
 * stub to handle SIGINT
 */
void close_server() {
    dr_on = FALSE;
    printf("\nClosing DR! Bye bye!\n");
    sem_close(&sem_db_file);
    sem_destroy(&sem_db_file);
    exit(EXIT_SUCCESS);
}

/**
 * Print usage in case of misbehaved input
 *
 * @param file_name path of the program
 */
void print_usage(const char *file_name) {
    printf("Usage: \"%s DR_PORT DR_ID\"\nwith:\n- DR_PORT: port in which the data repository has to be set up (>%d).\n- DR_ID: id of data repository (<8).\n",
           file_name, PORT_DELIMITER);
}

/**
 * Read the database file and get block ranges.
 *
 * @param db_path path of the file representing the database entries
 * @param filename file which range has to be extract
 * @return an array of unsigned long of length two in which in the first position is present the starting byte and in the second one the ending byte.
 */
unsigned long *get_block_dimensions_from_db_file(char *db_path, char *filename) {
    unsigned long *ret = (unsigned long *) malloc(sizeof(unsigned long) * 2);
    // Check for db file
    char data_info[BUFSIZ];
    char *file_entry;

    sem_wait(&sem_db_file);
    FILE *dbfp = fopen(db_path, "r");
    if (!dbfp) {
        fprintf(stderr,
                "%sCan't open configuration file at \"%s\".%s\n", KRED,
                db_path, KNRM);
        exit(EXIT_FAILURE);
    }

    while (fgets(data_info, sizeof(data_info), dbfp) != NULL && strlen(data_info) > 1) {
        file_entry = strtok(data_info, DB_FILE_DELIMITER);
        if (strncmp(filename, file_entry, strlen(file_entry)) == 0) {
            ret[0] = (unsigned long) strtol(strtok(NULL, DB_FILE_DELIMITER), NULL, 10);
            ret[1] = (unsigned long) strtol(strtok(NULL, DB_FILE_DELIMITER), NULL, 10);
            break;
        }
    }

    fclose(dbfp);
    sem_post(&sem_db_file);
    return ret;
}

/**
 * Append the new block entry to the database file
 *
 * @param database used just to retrieve dr id
 * @param block to gather the needed information
 */
void append_block_to_filedb(Grid_File_DB *database, Block_File *block) {
    char *db_path = (char *) malloc(sizeof(char) * FILENAME_MAX);
    snprintf(db_path, sizeof(db_path), "./%d.db", database->id);
    sem_wait(&sem_db_file);
    FILE *dbfp = fopen(db_path, "a");
    if (!dbfp) {
        fprintf(stderr,
                "%sCan't open configuration file at \"%s\".%s\n", KRED,
                db_path, KNRM);
        exit(EXIT_FAILURE);
    }
    fprintf(dbfp, "%s%s%lu%s%lu\n", block->block_name, DB_FILE_DELIMITER, block->start, DB_FILE_DELIMITER, block->end);
    fclose(dbfp);
    sem_post(&sem_db_file);
}

/**
 * Delete the block entry from the database file
 *
 * @param database used just to retrieve dr id
 * @param block_name name of the block that has to be removed
 */
void delete_entry_file_db(Grid_File_DB *database, char *block_name) {
    char data_info[BUFSIZ];
    char *file_entry;
    char *db_path = (char *) malloc(sizeof(char) * FILENAME_MAX);
    snprintf(db_path, sizeof(db_path), "./%d.db", database->id);
    sem_wait(&sem_db_file);
    FILE *dbfp = fopen(db_path, "r");
    FILE *temp = fopen("./temp.db", "w");
    if (!dbfp) {
        fprintf(stderr,
                "%sCan't open configuration file at \"%s\".%s\n", KRED,
                db_path, KNRM);
        exit(EXIT_FAILURE);
    }

    if (!temp) {
        fprintf(stderr,
                "%sCan't open configuration file at \"%s\".%s\n", KRED,
                "./temp.db", KNRM);
        exit(EXIT_FAILURE);
    }

    while (fgets(data_info, sizeof(data_info), dbfp) != NULL && strlen(data_info) > 1) {
        file_entry = strtok(data_info, DB_FILE_DELIMITER);
        if (strncmp(block_name, file_entry, strlen(file_entry)) == 0)
            continue;
        fputs(data_info, temp);
    }

    fclose(dbfp);
    fclose(temp);
    remove(db_path);
    rename("./temp.db", db_path);
    sem_post(&sem_db_file);
}

/**
 * DR routine to handle the incoming connection of the server and client
 *
 * @param args see t_args
 * @return NULL
 */
void *dr_routine(void *args) {
    thread_args *t_args = (thread_args *) args;
    int client_desc = t_args->client_desc;
    Grid_File_DB *database = t_args->file_db;
    size_t buffer_size = BUFSIZ;
    char buf[buffer_size];
    sig_atomic_t client_on = TRUE;
    while (dr_on && client_on) {
        recv_message(client_desc, buf);

        if (strncmp(buf, DR_UPDATE_MAP_CMD, strlen(DR_UPDATE_MAP_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *server_key = strtok(NULL, COMMAND_DELIMITER);
            if (check_key(server_key, SECRET_SERVER)) {
                craft_ack_response_header(buf, buffer_size);
                file_db_to_string(database, buf);
                strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
            } else {
                craft_nack_response(buf, buffer_size);
            }

            send_message(client_desc, buf, strlen(buf));
            break;
        } else if (strncmp(buf, REMOVE_CMD, strlen(REMOVE_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER); // we just ignore REMOVE_CMD header
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *file_name = strtok(NULL, COMMAND_DELIMITER);

            if (check_key(key, SECRET_SERVER) && remove_file(database, file_name))
                craft_ack_response(buf, buffer_size);
            else
                craft_nack_response(buf, buffer_size);

            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, TRANSFER_CMD, strlen(TRANSFER_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *server_key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            char *new_dr_id = strtok(NULL, COMMAND_DELIMITER);
            char *new_dr_ip = strtok(NULL, COMMAND_DELIMITER);
            char *new_dr_port = strtok(NULL, COMMAND_DELIMITER);

            if (check_key(server_key, SECRET_SERVER)) {

                craft_request_header(buf, TRANSFER_FROM_DR_CMD, buffer_size);
                char *dr_key = get_key(SECRET_DR);
                strncat(buf, dr_key, strlen(dr_key));
                strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
                strncat(buf, block_name, strlen(block_name));
                strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
                Block_File *block = get_block(database, block_name);
                unsigned long block_size = block->end - block->start;
                char size_str[BUFSIZ];
                snprintf(size_str, sizeof(size_str), "%lu", block_size);
                strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

                struct sockaddr_in repo_addr = {};

                int sock_d = socket(AF_INET, SOCK_STREAM, 0);
                ERROR_HELPER(sock_d, "Error in socket creation", TRUE);

                repo_addr.sin_addr.s_addr = inet_addr(new_dr_ip);
                repo_addr.sin_family = AF_INET;
                repo_addr.sin_port = htons(strtol(new_dr_port, NULL, 10));

                connect(sock_d, (struct sockaddr *) &repo_addr, sizeof(struct sockaddr_in));

                send_message(sock_d, buf, strlen(buf));

                recv_message(sock_d, buf);
                if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0) {
                    send_file(sock_d, block->block_name, block_size);
                    craft_ack_response(buf, buffer_size);
                    transfer_block(database, block_name, strtol(new_dr_id, NULL, 10));
                    delete_entry_file_db(database, block_name);
                } else {
                    craft_nack_response(buf, buffer_size);
                }
                close(sock_d);

            } else {
                craft_nack_response(buf, buffer_size);
            }
            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, TRANSFER_FROM_DR_CMD, strlen(TRANSFER_FROM_DR_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            char *block_size = strtok(NULL, COMMAND_DELIMITER);
            if (check_key(key, SECRET_DR)) {
                craft_ack_response(buf, buffer_size);
                send_message(client_desc, buf, strlen(buf));

                recv_file(client_desc, block_name, strtol(block_size, NULL, 10));
                transfer_block(database, block_name, database->id);
                delete_entry_file_db(database, block_name);
            } else {
                craft_nack_response(buf, buffer_size);
                send_message(client_desc, buf, strlen(buf));
            }
        } else if (strncmp(buf, GET_CMD, strlen(GET_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            if (check_key(key, SECRET_CLIENT)) {
                Block_File *block_file = get_block(database, block_name);

                craft_ack_response(buf, buffer_size);
                send_message(client_desc, buf, strlen(buf));

                send_file(client_desc, block_file->block_name, block_file->end - block_file->start);

            } else {
                craft_nack_response(buf, buffer_size);
                send_message(client_desc, buf, strlen(buf));
            }
        } else if (strncmp(buf, PUT_CMD, strlen(PUT_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strdup(strtok(NULL, COMMAND_DELIMITER));
            char *block_name = strdup(strtok(NULL, COMMAND_DELIMITER));
            unsigned long start = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);
            unsigned long end = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);

            if (check_key(key, SECRET_CLIENT)) {
                craft_ack_response(buf, buffer_size);
                send_message(client_desc, buf, strlen(buf));
                char *f_path = strdup(localpath);
                strncat(f_path, "/", strlen("/"));
                strcat(f_path, block_name);

                Block_File *block = new_block(f_path, database->id, start, end);
                recv_file(client_desc, f_path, end - start);
                f_path = localpath;
                char *file_name = get_file_name_from_block_name(block_name);
                strncat(f_path, "/", strlen("/"));
                strncat(f_path, file_name, strlen(file_name));

                if (!add_file(database, f_path, end - start, block)) {
                    // file already exists, just add block and update size of file
                    Grid_File *file = get_file(database, f_path);
                    file->size += (end - start);
                    append_block(file->head, block);
                }
                append_block_to_filedb(database, block);
                break;
            } else {
                craft_nack_response(buf, buffer_size);
                send_message(client_desc, buf, strlen(buf));
            }

        } else if (strncmp(buf, QUIT_CMD, strlen(QUIT_CMD)) == 0) {
            client_on = FALSE;
            db_destroyer(database);
        } else {
            printf("Command Unrecognized\n");
            fprintf(stderr, "%s", buf);

            craft_nack_response(buf, buffer_size);
            send_message(client_desc, buf, strlen(buf));
        }
    }
    pthread_exit(NULL);
}

/**
 * Checks the current state of the database and eventually populates it wit missing fields
 *
 * @param database
 */
void check_database_and_update(Grid_File_DB *database) {
    printf("Start updating database\n");

    struct stat st = {0};
    localpath = (char *) malloc(sizeof(char) * FILENAME_MAX);
    snprintf(localpath, sizeof(localpath), "./%d", database->id);

    if (stat(localpath, &st) == -1) {
        // database does not exist, let's create one with its relative entry and move on
        printf("No db found, I create a new one\n");
        mkdir(localpath, 0700);
        strcat(localpath, ".db");
        FILE *fp = fopen(localpath, "w");
        fclose(fp);
    } else {
        // scan folder database in search for files
        printf("DB found! Let's seek in \n");
        DIR *d;
        struct dirent *dir;
        d = opendir(localpath);
        char *db_filename = strdup(localpath);
        strcat(db_filename, ".db");
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name[0] != '.') { // ignore the hidden file and the back
                    char *block_path = strndup(localpath, strlen(localpath));
                    strcat(block_path, "/");
                    strcat(block_path, dir->d_name);

                    FILE *fp = fopen(block_path, "rb");
                    if (!fp) {
                        printf("%sA file present in the database file is not present into the directory, ignoring..%s\n",
                               KRED,
                               KNRM);
                        continue;
                    }
                    fseek(fp, 0, SEEK_END);
                    long unsigned length = ftell(fp);
                    printf("%s, size: %lu\n", block_path, length);

                    char *filename = get_file_name_from_block_name(block_path);
                    Grid_File *existing = get_file(database, filename);
                    unsigned long *range = get_block_dimensions_from_db_file(localpath, filename);
                    Block_File *block = new_block(block_path, database->id, range[0], range[1]);

                    if (!existing)
                        add_file(database, filename, length, block);
                    else
                        append_block(existing->head, block);

                    fclose(fp);
                }
            }
            closedir(d);
        }
    }
}


/**
 * Checks the internal db of the repository and launches the client handlers
 */
void start_dr_routine(int port, u_int8_t id) {
    // open a socket and wait for auth from server
    int dr_sock = server_init(port);
    int client_desc, ret;

    pthread_t thread;
    thread_args *t_args;

    Grid_File_DB *database = init_db(TRUE, id);

    check_database_and_update(database);

    printf("Updating complete, starting dr routine...\n");

    while (dr_on) {
        client_desc = accept(dr_sock, NULL, NULL);
        ERROR_HELPER(client_desc, "Error on accepting incoming connection", TRUE);
        if (client_desc < 0) continue;
        printf("New entity connected!\n");
        t_args = (thread_args *) malloc(sizeof(thread_args));
        t_args->client_desc = client_desc;
        t_args->file_db = database;
        ret = pthread_create(&thread, NULL, dr_routine, (void *) t_args);
        PTHREAD_ERROR_HELPER(ret, "Error on pthread creation", TRUE);
        ret = pthread_detach(thread);
        PTHREAD_ERROR_HELPER(ret, "Error on thread detaching", TRUE);
    }
}

/**
 * Usage: "./program_name DR_PORT DR_ID"
 * with:
 * @param DR_PORT: port in which the data repository has to be set up (>%d).
 * @param DR_ID: id of data repository (<8).
 *
 */
int main(int argc, char const *argv[]) {
    if (argc == 3) {
        int dr_port = (int) strtol(argv[1], NULL, 10);
        u_int8_t id = strtol(argv[2], NULL, 10);
        if (dr_port <= PORT_DELIMITER) {
            printf("Invalid port number!\n");
            print_usage(argv[0]);
        } else {

            struct sigaction sigint_action;
            sigset_t block_mask;

            sigfillset(&block_mask);
            sigint_action.sa_handler = close_server;
            sigint_action.sa_mask = block_mask;
            sigint_action.sa_flags = 0;
            int ret = sigaction(SIGINT, &sigint_action, NULL);
            ERROR_HELPER(ret, "Error on arming SIGINT: ", TRUE);

            signal(SIGPIPE, SIG_IGN);
            sem_init(&sem_db_file, 0, CRITICAL_SECTION_DB_SIZE);
            start_dr_routine(dr_port, id);
        }
    } else {
        print_usage(argv[0]);
    }
    return 0;
}
