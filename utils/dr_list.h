//
// Created by thecave3 on 18/09/19.
//

#ifndef GRID_FTP_DR_LIST_H
#define GRID_FTP_DR_LIST_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_LEN_IP 32

typedef struct Node {
    u_int8_t id;
    char *ip;
    u_int16_t port;
    struct Node *next;
} Node;

typedef struct DR_List {
    int size; // size of the list
    Node *node; // head node of the list
} DR_List;


DR_List *new_list();

Node *new_node(u_int8_t id, char *ip, u_int16_t port);

void print_list(DR_List *list);

char *list_to_string(DR_List *list);

void append_to_list(DR_List *list, Node *node);

Node *append_node(Node *head, Node *new_node);

void delete_node(DR_List *list, u_int8_t id);


#endif //GRID_FTP_DR_LIST_H
