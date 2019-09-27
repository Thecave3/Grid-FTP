//
// Created by Andrea Lacava on 18/09/19.
//

#include "data_repository.h"

sig_atomic_t dr_on = 1;

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

    while (dr_on) { // TODO check close cases
        recv_message(client_desc, buf);

        if (strncmp(buf, DR_UPDATE_MAP_CMD, strlen(DR_UPDATE_MAP_CMD)) == 0) {
            craft_ack_response(buf);
            send_message(client_desc, buf, strlen(buf));
            break;
        } else if (strncmp(buf, REMOVE_CMD, strlen(REMOVE_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER); // we just ignore REMOVE_CMD header
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);

            if (check_key(key) && remove_block(file_db, block_name))
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


            if (check_key(key) && transfer_block(file_db, block_name, new_dr_id)) {
                // TODO send block file to new_dr_ip new_dr_port
                // TODO send a TRANSFER_FROM_DR_CMD request to new_dr_ip, new_dr_port

                craft_ack_response(buf);
            } else {
                craft_nack_response(buf);
            }
            send_message(client_desc, buf, strlen(buf));

        } else if (strncmp(buf, TRANSFER_FROM_DR_CMD, strlen(TRANSFER_FROM_DR_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            // TODO send ack
            // TODO receive file
            transfer_block(file_db, block_name,) // TODO where do i get my id?

        } else if (strncmp(buf, GET_CMD, strlen(GET_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            if (check_key(key)) {
                Block_File *block_file = get_block(file_db, block_name);
                // block_file->fp // TODO file pointer to FILE* that has to be active in dr
                craft_ack_response(buf);
                send_message(client_desc, buf, strlen(buf));
                // TODO send file to client

            } else {
                craft_nack_response(buf);
                send_message(client_desc, buf, strlen(buf));
            }
        } else if (strncmp(buf, PUT_CMD, strlen(PUT_CMD)) == 0) {
            strtok(buf, COMMAND_DELIMITER);
            char *key = strtok(NULL, COMMAND_DELIMITER);
            char *block_name = strtok(NULL, COMMAND_DELIMITER);
            if (check_key(key)) {
                craft_ack_response(buf);
                send_message(client_desc, buf, strlen(buf));
                // TODO receive and store the file
            }

        } else if (strncmp(buf, QUIT_CMD, strlen(QUIT_CMD)) == 0) {

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
