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

void *print_list(DR_List *list) {
    printf("Printlist");
    for (Node *node = list->node; node->next != NULL; node = node->next)
        printf("id:%d, ip: %s\n", node->id, node->ip);

}


Node *new_node(int id, char *ip) {
    Node *node = (Node *) malloc(sizeof(Node));
    node->id = id;
    node->ip = ip;
    node->next = NULL;
    return node;
}

void *append_to_list(DR_List *list, Node *node) {
    Node *head = list->node;
    list->size++;
    if (head == NULL)
        return head = node;
    else
        return append_node(head, node);
}

void *append_node(Node *head, Node *new_node) {
    if (head->next == NULL) {
        return head->next = new_node;
    }
    return append_node(head->next, new_node);
}
