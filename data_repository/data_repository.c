//
// Created by Andrea Lacava on 18/09/19.
//

#include "data_repository.h"

#define PORT_DELIMITER 1024 // just to preserve reserved port

void print_usage(const char *file_name) {
    printf("Usage: \"%s DR_PORT\"\nwith:\n- DR_PORT: port in which the data repository has to be set up (>%d).\n",
           file_name, PORT_DELIMITER);
}

void start_dr_routine(int port) {
    // open a socket and wait for auth from server
    int dr_sock = server_init(port);
    char buf[BUFSIZ];
    int client_desc, ret;
    while (1) {
        client_desc = accept(dr_sock, NULL, NULL);
        ERROR_HELPER(client_desc, "Error on accepting incoming connection", FALSE);

        ret = recv_message(client_desc, buf);
        printf("%s", buf);
        // TODO Authentication phase

        if (strncmp(buf, DR_UPDATE_MAP, strlen(DR_UPDATE_MAP)) == 0) {
            strncpy(buf, OK_RESPONSE, strlen(OK_RESPONSE));
            send_message(client_desc, buf, strlen(OK_RESPONSE));
            break;
        }
    }
}

int main(int argc, char const *argv[]) {
    if (argc == 2) {
        int dr_port = (int) strtol(argv[1], NULL, 10);
        if (dr_port <= PORT_DELIMITER) {
            printf("Invalid port number!\n");
            print_usage(argv[0]);
            return 0;
        } else {
            start_dr_routine(dr_port);
        }
    } else {
        print_usage(argv[0]);
        return 0;
    }
    return 0;
}
