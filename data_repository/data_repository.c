//
// Created by Andrea Lacava on 18/09/19.
//

#include "data_repository.h"

sig_atomic_t dr_on = TRUE;

typedef struct thread_args {
    int client_desc;
    //DR_List *list;
    Grid_File_DB *file_db;
} thread_args;

void print_usage(const char *file_name) {
    printf("Usage: \"%s DR_PORT\"\nwith:\n- DR_PORT: port in which the data repository has to be set up (>%d).\n",
           file_name, PORT_DELIMITER);
}

int check_key(char *key) {
    // TODO
    return FALSE;
}

void *dr_routine(void *args) {
    thread_args *t_args = (thread_args *) args;
    int client_desc = t_args->client_desc;
    Grid_File_DB *file_db = t_args->file_db;
    char buf[BUFSIZ];
    sig_atomic_t client_on = TRUE;
    while (dr_on && client_on) {
        recv_message(client_desc, buf);

        if (strncmp(buf, DR_UPDATE_MAP_CMD, strlen(DR_UPDATE_MAP_CMD)) == 0) {
            craft_ack_response(buf);
            send_message(client_desc, buf, strlen(buf));
            break;
        } else if (strncmp(buf, REMOVE_CMD, strlen(REMOVE_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER); // we just ignore REMOVE_CMD header
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *file_name = strtok(NULL, COMMAND_DELIMITER);

            if (check_key(key) && remove_file(file_db, file_name))
                craft_ack_response(buf);
            else
                craft_nack_response(buf);

            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, TRANSFER_CMD, strlen(TRANSFER_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            char *new_dr_id = strtok(NULL, COMMAND_DELIMITER);
            char *new_dr_ip = strtok(NULL, COMMAND_DELIMITER);
            char *new_dr_port = strtok(NULL, COMMAND_DELIMITER);


            if (check_key(key) && transfer_block(file_db, block_name, strtol(new_dr_id, NULL, 10))) {

                craft_request_header(buf, TRANSFER_FROM_DR_CMD);
                char *key = get_dr_key();
                strncat(buf, key, strlen(key));
                strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
                strncat(buf, block_name, strlen(block_name));
                strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
                Block_File *block = get_block(file_db, block_name);
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

                    craft_ack_response(buf);

                } else {
                    craft_nack_response(buf);
                }
                close(sock_d);

            } else {
                craft_nack_response(buf);
            }
            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, TRANSFER_FROM_DR_CMD, strlen(TRANSFER_FROM_DR_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            char *block_size = strtok(NULL, COMMAND_DELIMITER);
            if (check_key(key, SECRET_DR)) {
                craft_ack_response(buf);
                send_message(client_desc, buf, strlen(buf));

                FILE *new_block = recv_file(client_desc, block_name, strtol(block_size, NULL, 10));
                transfer_block(file_db, block_name, file_db->id);

            } else {
                craft_nack_response(buf);
                send_message(client_desc, buf, strlen(buf));
            }
        } else if (strncmp(buf, GET_CMD, strlen(GET_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            if (check_key(key)) {
                Block_File *block_file = get_block(file_db, block_name);

                craft_ack_response(buf);
                send_message(client_desc, buf, strlen(buf));

                send_file(client_desc, block_file->block_name, block_file->end - block_file->start);

            } else {
                craft_nack_response(buf);
                send_message(client_desc, buf, strlen(buf));
            }
        } else if (strncmp(buf, PUT_CMD, strlen(PUT_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            unsigned long start = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);
            unsigned long end = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);

            if (check_key(key)) {
                craft_ack_response(buf);
                send_message(client_desc, buf, strlen(buf));

                Block_File *block = new_block(block_name, file_db->id, start, end);
                recv_file(client_desc, block_name, end - start);
                char *file_name = get_file_name_from_block_name(block_name);
                if (add_file(file_db, file_name, end - start, block)) {
                    // file already exists, just add block and update size of file
                    Grid_File *file = get_file(file_db, file_name);
                    file->size += (end - start);
                    append_block(file->head, block);
                }
            } else {
                craft_nack_response(buf);
                send_message(client_desc, buf, strlen(buf));
            }

        } else if (strncmp(buf, QUIT_CMD, strlen(QUIT_CMD)) == 0) {
            client_on = FALSE;
        } else {
            craft_nack_response(buf);
            send_message(client_desc, buf, strlen(buf));
            perror("Command Unrecognized\n");
            fprintf(stderr, "%s", buf);
        }

        //            craft_ack_response(buf);
        //            FILE *fp = recv_file(client_desc, file_name, file_size);

    }
    pthread_exit(NULL);
}


void start_dr_routine(int port) {
    // open a socket and wait for auth from server
    int dr_sock = server_init(port);
    int client_desc, ret;

    pthread_t thread;
    thread_args *t_args;

    while (1) {
        client_desc = accept(dr_sock, NULL, NULL);
        ERROR_HELPER(client_desc, "Error on accepting incoming connection", TRUE);
        if (client_desc < 0) continue;
        t_args = (thread_args *) malloc(sizeof(thread_args));
        //t_args->list = list;
        t_args->client_desc = client_desc;
        ret = pthread_create(&thread, NULL, dr_routine, (void *) t_args);
        PTHREAD_ERROR_HELPER(ret, "Error on pthread creation", TRUE);
        ret = pthread_detach(thread);
        PTHREAD_ERROR_HELPER(ret, "Error on thread detaching", TRUE);
    }
}

int main(int argc, char const *argv[]) {
    if (argc == 2) {
        int dr_port = (int) strtol(argv[1], NULL, 10);
        if (dr_port <= PORT_DELIMITER) {
            printf("Invalid port number!\n");
            print_usage(argv[0]);
        } else {
            signal(SIGPIPE, SIG_IGN);

            start_dr_routine(dr_port);
        }
    } else {
        print_usage(argv[0]);
    }
    return 0;
}
