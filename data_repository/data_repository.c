//
// Created by Andrea Lacava on 18/09/19.
//

#include "data_repository.h"

void print_usage(const char *file_name) {
    printf("Usage: \"%s DR_PORT\"\nwith:\n- DR_PORT: port in which the data repository has to be set up (>%d).\n",
           file_name, PORT_DELIMITER);
}

void start_dr_routine(int port) {
    // open a socket and wait for auth from server
    int dr_sock = server_init(port);
    char buf[BUFSIZ];
    int client_desc;
    while (1) {
        client_desc = accept(dr_sock, NULL, NULL);
        ERROR_HELPER(client_desc, "Error on accepting incoming connection", TRUE);

        recv_message(client_desc, buf);
        if (strncmp(buf, DR_UPDATE_MAP_CMD, strlen(DR_UPDATE_MAP_CMD)) == 0) {
            strncpy(buf, OK_RESPONSE, strlen(OK_RESPONSE));
            send_message(client_desc, buf, strlen(OK_RESPONSE));
            break;
        } else {
            perror("Command Unrecognized\n");
            fprintf(stderr, "%s", buf);
        }
    }
}

int main(int argc, char const *argv[]) {
    if (argc == 2) {
        int dr_port = (int) strtol(argv[1], NULL, 10);
        if (dr_port <= PORT_DELIMITER) {
            printf("Invalid port number!\n");
            print_usage(argv[0]);
        } else {
            start_dr_routine(dr_port);
        }
    } else {
        print_usage(argv[0]);
    }
    return 0;
}
