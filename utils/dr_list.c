//
// Created by Andrea Lacava on 18/09/19.
//

#include "dr_list.h"
#include <string.h>
#include <protocol.h>

DR_List *new_list() {
    DR_List *list = (DR_List *) malloc(sizeof(DR_List));
    list->size = 0;
    list->node = NULL;
    return list;
}


void print_list(DR_List *list) {
    printf("************************\n");
    printf("Repository list:\n");
    for (Node *node = list->node; node; node = node->next)
        printf("Repository number %d, ip: %s, port: %hu\n", node->id, node->ip, node->port);
    printf("************************\n");
}


//id,ip,port
char *list_to_string(DR_List *list) {
    int current_size = 0;
    char *buf = (char *) calloc(sizeof(char), BUFSIZ);

    for (Node *node = list->node; node; node = node->next) {
        current_size += snprintf(buf + current_size, BUFSIZ, "%d%s%s%s%d", node->id, COMMAND_DELIMITER, node->ip,
                                 COMMAND_DELIMITER, node->port);
        if (current_size >= BUFSIZ)
            buf = realloc(buf, 2 * BUFSIZ);

        if (node->next) {
            strncat(buf, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
            current_size++;
        }
    }

    return buf;
}


Node *new_node(u_int8_t id, char *ip, u_int16_t port) {
    Node *node = (Node *) malloc(sizeof(Node));
    node->id = id;
    node->ip = (char *) malloc(MAX_LEN_IP * sizeof(char));
    strncpy(node->ip, ip, MAX_LEN_IP);
    node->port = port;
    node->next = NULL;
    return node;
}


Node *append_node(Node *head, Node *new_node) {
    if (!head)
        return new_node;
    head->next = append_node(head->next, new_node);
    return head;
}

void append_to_list(DR_List *list, Node *node) {
    list->size++;
    list->node = append_node(list->node, node);
}

Node *_get_node(Node *node, u_int8_t id) {
    if (!node)
        return node;

    if (node->id == id)
        return node;

    return _get_node(node->next, id);
}

Node *get_node(DR_List *list, u_int8_t id) {
    return _get_node(list->node, id);
}

void delete_node(DR_List *list, u_int8_t id) {
    Node *temp = list->node;
    Node *pre = NULL;

    if (id == temp->id) {
        list->size--;
        list->node = temp->next;
        free(temp);
        return;
    }

    while (temp && temp->id != id) {
        pre = temp;
        temp = temp->next;
    }

    if (!temp)
        return;

    list->size--;
    pre->next = temp->next;
    free(temp);
}
