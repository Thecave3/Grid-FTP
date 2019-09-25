#include "client.h"

sig_atomic_t keep_alive = 1;


void commands_available() {
    printf("Commands available:\n");
    printf("- \"%s\": clear the screen.\n", CLEAR);
    printf("- \"%s\": print this list.\n", HELP);
    printf("- \"%s\": put new file in data repositories.\n", PUT);
    printf("- \"%s\": get file from data repositories.\n", GET);
    printf("- \"%s\": remove file from data repositories.\n", REMOVE);
    printf("- \"%s\": remove file from data repositories.\n", EXIT_CLIENT);
}


int client_init() {
    int ret;
    struct sockaddr_in repo_addr = {};

    int sock_d = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(sock_d, "Error in socket creation", TRUE);

    repo_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    repo_addr.sin_family = AF_INET;
    repo_addr.sin_port = htons(SERVER_PORT);
    ret = connect(sock_d, (struct sockaddr *) &repo_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Error in connection", TRUE);

    return sock_d;
}

char *authentication(int client_desc) {
    char username[MAX_LEN_UNAME];
    char password[MAX_LEN_PWD];
    char buf[BUFSIZ];
    int ret;

    // TODO username and password as argument
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

    if (strncmp(buf, OK_RESPONSE, strlen(OK_RESPONSE)) == 0)
        return strtok(buf + strlen(OK_RESPONSE), COMMAND_DELIMITER);
    else
        return NULL;

}


void quit_command(int client_desc) {
    send_message(client_desc, QUIT_CMD, strlen(QUIT_CMD));
    close(client_desc);
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
        } else if (strncmp(buf, PUT, strlen(PUT)) == 0) {

//            craft_request_header(buf, PUT_CMD);
//            strncat(buf, file_name, strlen(file_name));
//            strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
//            strncat(buf, file_size, strlen(file_size));
//            strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));
//
//            send_message(socket_desc, buf, strlen(buf));
//
//
//            craft_ack_response(buf);
//            send_message(client_desc, buf, strlen(buf));
//            FILE *fp = recv_file(client_desc, file_name, file_size);
            // TODO divide file in blocks and end PUT_CMD

        } else if (strncmp(buf, GET, strlen(GET)) == 0) {


        } else if (strncmp(buf, REMOVE, strlen(REMOVE)) == 0) {


        } else if (strncmp(buf, EXIT_CLIENT, strlen(EXIT_CLIENT)) == 0) {
            quit_command(client_desc);
            printf("Connection closed, bye bye!");
            keep_alive = 0;
        } else {
            printf("%s", buf);
            printf("%sError unrecognized command\n%s", KRED, KNRM);
            commands_available();
        }

    }

}


DR_List *get_data_repositories(int client_desc, char *key) {
    char buf[BUFSIZ];

    craft_request_header(buf, GET_DR_CMD);
    strncat(buf, key, strlen(key));
    strncat(buf, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR));

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

int main(/*int argc, char const *argv[]*/) {

    int client_desc = client_init();

    char *key = authentication(client_desc);
    if (!key) {
        perror("Authentication failed!\nExiting...\n");
        quit_command(client_desc);
        return 0;
    }

    DR_List *list = get_data_repositories(client_desc, key);
    if (!list) {
        perror("Error in the creation of the list, exiting");
        quit_command(client_desc);
        return -1;
    }

    printf("%sGrid FTP Client, type a command:%s\n", KBLU, KNRM);

    client_routine(client_desc, key, list);

    return 0;
}