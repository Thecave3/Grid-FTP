//
// Created by Andrea Lacava on 18/09/19.
//

#include "dr_list.h"

DR_List *new_list() {
    DR_List *list = (DR_List *) malloc(sizeof(DR_List));
    list->size = 0;
    list->node = NULL;
    return list;
}

void print_list(DR_List *list) {
    printf("************************\n");

    for (Node *node = list->node; node; node = node->next)
        printf("Repository number %d, ip: %s, port: %hu\n", node->id, node->ip, node->port);

    printf("************************\n");
}


Node *new_node(int id, char *ip, u_int16_t port) {
    Node *node = (Node *) malloc(sizeof(Node));
    node->id = id;
    node->ip = ip;
    node->port = (u_int16_t) port;
    node->next = NULL;
    return node;
}

void append_to_list(DR_List *list, Node *node) {
    list->size++;
    if (list->node == NULL) {
        list->node = node;
        return;
    } else
        return append_node(list->node, node);
}

void append_node(Node *head, Node *new_node) {
    if (head->next == NULL) {
        head->next = new_node;
        return;
    }
    return append_node(head->next, new_node);
}
