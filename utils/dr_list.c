//
// Created by Andrea Lacava on 18/09/19.
//

#include "dr_list.h"
#include <string.h>
#include <protocol.h>

/**
 * Constructor like for the data repository linked list.
 *
 * @return an empty data repository list
 */
DR_List *new_list() {
    DR_List *list = (DR_List *) malloc(sizeof(DR_List));
    list->size = 0;
    list->node = NULL;
    return list;
}

/**
 * Prints the list to screen.
 * Just used to debug.
 *
 * @param list to be printed
 */
void print_list(DR_List *list) {
    printf("************************\n");
    printf("Repository list:\n");
    for (DR_Node *node = list->node; node; node = node->next)
        printf("Repository number %d, ip: %s, port: %hu\n", node->id, node->ip, node->port);
    printf("************************\n");
}

/**
 * ToString of the list
 *
 * @param list to be put on string
 * @return string of the list with this format <id,ip,port>
 */
char *list_to_string(DR_List *list) {
    int current_size = 0;
    char *buf = (char *) calloc(sizeof(char), BUFSIZ);

    for (DR_Node *node = list->node; node; node = node->next) {
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

/**
 * Constructor for a new node representing a single data repository.
 * This is called just in the initial parse of the list.
 * In a more updated version a dr that comes online after server's one will send a specific command to be added to server and DR_Lists.
 *
 *
 * @param id of te dr
 * @param ip of the dr
 * @param port of the dr
 * @return a new Node* struct
 */
DR_Node *new_node(u_int8_t id, char *ip, u_int16_t port) {
    DR_Node *node = (DR_Node *) malloc(sizeof(DR_Node));
    node->id = id;
    node->ip = (char *) malloc(MAX_LEN_IP * sizeof(char));
    strncpy(node->ip, ip, MAX_LEN_IP);
    node->port = port;
    node->next = NULL;
    return node;
}

/**
 * Append the DR_Node* new_node at the end of the list that begins with *head.
 *
 * @param head of the list
 * @param new_node to be put at the end
 * @return the head of the list.
 */
DR_Node *append_node(DR_Node *head, DR_Node *new_node) {
    if (!head)
        return new_node;
    head->next = append_node(head->next, new_node);
    return head;
}

/**
 * Add the DR_Node *node to the end of the DR_List* list.
 *
 * @param list
 * @param node
 */
void append_to_list(DR_List *list, DR_Node *node) {
    list->size++;
    list->node = append_node(list->node, node);
}

DR_Node *_get_node(DR_Node *node, u_int8_t id) {
    if (!node)
        return node;

    if (node->id == id)
        return node;

    return _get_node(node->next, id);
}

/**
 * Search for a node with the specific id in the list
 *
 * @param list of dr_repositories active
 * @param id to be find inside the list
 * @return a pointer to the DR_Node if it present on the list, NULL otherwise.
 */
DR_Node *get_node(DR_List *list, u_int8_t id) {
    return _get_node(list->node, id);
}

/**
 * Delete a node with ID id that probably wasn't online when requested.
 *
 * @param list of active dr
 * @param id of the node to be eliminated
 */
void delete_node(DR_List *list, u_int8_t id) {
    DR_Node *temp = list->node;
    DR_Node *pre = NULL;

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
