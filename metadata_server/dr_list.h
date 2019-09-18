//
// Created by thecave3 on 18/09/19.
//

#ifndef GRID_FTP_DR_LIST_H
#define GRID_FTP_DR_LIST_H

#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int id;
    char *ip;
    struct Node *next;
} Node;

typedef struct DR_List {
    int size; // size of the list
    Node *node; // head node of the list
} DR_List;


DR_List *new_list();

Node *new_node(int id, char *ip);

void print_list(DR_List *list);

void append_to_list(DR_List *list, Node *node);

void append_node(Node *head, Node *new_node);


#endif //GRID_FTP_DR_LIST_H
