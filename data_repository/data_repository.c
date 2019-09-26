//
// Created by Andrea Lacava on 18/09/19.
//

#include "data_repository.h"

typedef struct thread_args {
    int client_desc;
    //DR_List *list;
} thread_args;

void print_usage(const char *file_name) {
    printf("Usage: \"%s DR_PORT\"\nwith:\n- DR_PORT: port in which the data repository has to be set up (>%d).\n",
           file_name, PORT_DELIMITER);
}

void *dr_routine(void *args) {
    thread_args *t_args = (thread_args *) args;
    int client_desc = t_args->client_desc;
    char buf[BUFSIZ];

    while (1) {
        recv_message(client_desc, buf);

        if (strncmp(buf, DR_UPDATE_MAP_CMD, strlen(DR_UPDATE_MAP_CMD)) == 0) {
            craft_ack_response(buf);
            send_message(client_desc, buf, strlen(buf));
            break;
        } else if (strncmp(buf, REMOVE_CMD, strlen(REMOVE_CMD)) == 0) {

        } else if (strncmp(buf, TRANSFER_CMD, strlen(TRANSFER_CMD)) == 0) {

        } else if (strncmp(buf, GET_CMD, strlen(GET_CMD)) == 0) {

        } else if (strncmp(buf, PUT_CMD, strlen(PUT_CMD)) == 0) {

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
