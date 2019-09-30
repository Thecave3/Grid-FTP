//
// Created by Andrea Lacava on 26/09/19.
//

#include "file_db.h"
#include "common.h"
#include "protocol.h"
#include <semaphore.h>

#define CRITICAL_SECTION_SIZE 1
sem_t sem;

Grid_File_DB *init_db(int is_dr, u_int8_t id) {
    Grid_File_DB *database = (Grid_File_DB *) malloc(sizeof(Grid_File_DB));
    database->is_dr = is_dr;
    database->id = id;
    database->head = NULL;
    sem_init(&sem, 0, CRITICAL_SECTION_SIZE);
    return database;
}

void file_db_to_string(Grid_File_DB *database, char *buffer) {
    char *file_str;
    sem_wait(&sem);
    for (Grid_File *file = database->head; file; file = file->next) {
        file_str = file_to_string(file);
        strncat(buffer, file_str, strlen(file_str));
        if (file->next)
            strncat(buffer, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    }
    sem_post(&sem);
}

int is_a_block_of_file(char *filename, char *blockname) {
    if (!filename || !blockname)
        return FALSE;
    char *block_radix = strtok(blockname, FILE_BLOCK_SEPARATOR);
    return strncmp(block_radix, filename, strlen(filename));
}

// F,name,size,B,block_name,dr_id,start,end
void update_file_db_from_string(Grid_File_DB *database, char *buffer) {
    char *filename = NULL;
    long unsigned file_size = 0;
    u_int8_t dr_id = 0;
    long unsigned start = 0;
    long unsigned end = 0;
    Grid_File *file = NULL;
    char *delimiter = NULL;

    strtok(buffer, COMMAND_DELIMITER); // OK RESPONSE
    delimiter = strtok(NULL, COMMAND_DELIMITER);

    if (strncmp(delimiter, COMMAND_TERMINATOR, strlen(COMMAND_TERMINATOR)) == 0) {
        printf("No file present in the dr, moving forward\n");
        return;
    }

    while (delimiter) {
        if (strncmp(delimiter, FILE_DELIMITER, strlen(FILE_DELIMITER)) == 0) {
            filename = strtok(NULL, COMMAND_DELIMITER);
            file = get_file(database, filename);
            file_size = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);
            if (!file)
                add_file(database, filename, file_size, NULL);

        } else if (strncmp(delimiter, BLOCK_DELIMITER, strlen(BLOCK_DELIMITER)) == 0) {
            filename = strtok(NULL, COMMAND_DELIMITER);
            dr_id = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);
            start = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);
            end = strtol(strtok(NULL, COMMAND_DELIMITER), NULL, 10);
            Block_File *block = new_block(filename, dr_id, start, end);
            file = get_file(database, get_file_name_from_block_name(filename));
            append_block(file->head, block);

        } else {
            fprintf(stderr, "Error in parsing, ignoring  delimiter: \"%s\".\n", delimiter);
        }

        delimiter = strtok(NULL, COMMAND_DELIMITER);
    }
}


static int file_compare(Grid_File *file, char *n) {
    size_t fstr_size = strlen(file->name);
    size_t n_size = strlen(n);
    if (fstr_size != n_size)
        return FALSE;
    if (strncmp(file->name, n, fstr_size) == 0)
        return TRUE;
    return FALSE;
}

static int block_compare(Block_File *block, char *n) {
    size_t bstr_size = strlen(block->block_name);
    size_t n_size = strlen(n);
    if (bstr_size != n_size)
        return FALSE;
    if (strncmp(block->block_name, n, bstr_size) == 0)
        return TRUE;
    return FALSE;
}

static u_int8_t find_file(Grid_File_DB *db, char *name) {
    for (Grid_File *file = db->head; file; file = file->next)
        if (file_compare(file, name))
            return TRUE;
    return FALSE;
}

static Grid_File *get_tail(Grid_File_DB *db) {
    Grid_File *t = db->head;
    while (t->next)
        t = t->next;
    return t;
}


Grid_File *_get_file(Grid_File *file, char *name) {
    if (!file)
        return file;

    if (file_compare(file, name))
        return file;

    return _get_file(file->next, name);
}

Grid_File *get_file(Grid_File_DB *database, char *name) {
    sem_wait(&sem);
    Grid_File *result = _get_file(database->head, name);
    sem_post(&sem);
    return result;
}

Grid_File *new_file(char *name, unsigned long size, Block_File *head_block) {
    Grid_File *file = (Grid_File *) malloc(sizeof(Grid_File));
    file->name = name;
    file->head = head_block;
    file->size = size;
    file->next = NULL;
    return file;
}

int add_file(Grid_File_DB *database, char *name, long unsigned size, Block_File *head) {
    sem_wait(&sem);
    if (find_file(database, name))
        return FALSE;
    if (!database->head) {
        database->head = new_file(name, size, head);
    } else {
        // We already know that file does not exists in the db
        Grid_File *tail_list = get_tail(database);
        tail_list->next = new_file(name, size, head);
    }
    sem_post(&sem);
    return TRUE;
}

int _remove_file(Grid_File *file, char *name, int is_dr, u_int8_t dr_id) {

    if (!file)
        return TRUE;

    if (file_compare(file, name)) {

        Block_File *temp = NULL;
        int ret;

        for (Block_File *block = file->head; block; block = block->next) {
            temp = block->next;
            if (is_dr && block->dr_id == dr_id) {
                ret = remove(block->block_name);
                ERROR_HELPER(ret, "Error on file remove", FALSE);
            }
            free(block);
            block = temp;
        }
        free(temp);
        return TRUE;
    }
    return _remove_file(file->next, name, is_dr, dr_id);
}

int remove_file(Grid_File_DB *database, char *name) {

    sem_wait(&sem);
    int result = _remove_file(database->head, name, database->is_dr, database->id);
    sem_post(&sem);
    return result;
}

// duplicate the string
// split the string at the separator in order to extract the file_name
// Output string is dynamically allocated. Remember to FREE.
char *get_file_name_from_block_name(char *block_name) {

    char *b_str = strndup(block_name, strlen(block_name));
    char *f_str = strtok(b_str, FILE_BLOCK_SEPARATOR);
    return f_str;
}

//B,block_name,dr_id,start,end
void block_to_string(char *result, Block_File *block) {
    char dr_id[FILE_SIZE_LIMIT]; // Overpowered probably
    char start[FILE_SIZE_LIMIT];
    char end[FILE_SIZE_LIMIT];

    strncat(result, BLOCK_DELIMITER, strlen(BLOCK_DELIMITER));
    strncat(result, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    strncat(result, block->block_name, strlen(block->block_name));
    strncat(result, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    snprintf(dr_id, sizeof(dr_id), "%d", block->dr_id);
    strncat(result, dr_id, strlen(dr_id));
    strncat(result, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    snprintf(start, sizeof(start), "%lu", block->start);
    strncat(result, start, strlen(start));
    strncat(result, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    snprintf(end, sizeof(end), "%lu", block->end);
    strncat(result, end, strlen(end));

}

//F,name,size,block_to_string
char *file_to_string(Grid_File *file) {
    char *result = (char *) malloc(sizeof(char) * BUFSIZ);
    char size[FILENAME_MAX];
    strncat(result, FILE_DELIMITER, strlen(FILE_DELIMITER));
    strncat(result, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    strncat(result, file->name, strlen(file->name));
    strncat(result, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    snprintf(size, sizeof(size), "%lu", file->size);
    strncat(result, size, strlen(size));
    strncat(result, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));

    for (Block_File *block = file->head; block; block = block->next) {
        block_to_string(result, block);
        if (block->next)
            strncat(result, COMMAND_DELIMITER, strlen(COMMAND_DELIMITER));
    }

    return result;
}

Block_File *new_block(char *block_name, u_int8_t dr_id, unsigned long start, unsigned long end) {
    Block_File *result = (Block_File *) malloc(sizeof(Block_File));
    result->block_name = block_name;
    result->dr_id = dr_id;
    result->start = start;
    result->end = end;
    result->next = NULL;

    return result;
}

Block_File *_append_block(Block_File *head, Block_File *block_new) {
    if (!head)
        return block_new;
    head->next = _append_block(head->next, block_new);
    return head;
}

Block_File *append_block(Block_File *head, Block_File *block_new) {
    sem_wait(&sem);
    Block_File *result = _append_block(head, block_new);
    sem_post(&sem);
    return result;
}

Block_File *_get_block(Grid_File *file, char *block_name) {
    Grid_File *temp = file;

    while (temp) {
        for (Block_File *block = temp->head; block; block = block->next) {
            if (strncmp(block->block_name, block_name, strlen(block->block_name)) == 0 &&
                strlen(block_name) == strlen(block->block_name))
                return block;
        }

        temp = temp->next;
    }

    return NULL;
}

Block_File *get_block(Grid_File_DB *file_db, char *block_name) {
    return _get_block(file_db->head, block_name);
}


int _remove_block(Grid_File *file, char *block_name, u_int8_t caller_id) {
    int ret = 0;
    Block_File *prec_b = NULL;
    for (Block_File *b = file->head; b; b = b->next) {
        if (block_compare(b, block_name)) {
            // update the file list
            if (b == file->head)
                file->head = b->next;
            else
                prec_b->next = b->next;
            // remove the block
            if (b->dr_id == caller_id) {
                ret = remove(b->block_name);
                ERROR_HELPER(ret, "Error on block remove", FALSE);
                free(b);
                return TRUE;
            }
        }
        prec_b = b;
    }
    return FALSE;
}

int remove_block(Grid_File_DB *file_db, char *block_name) {
    sem_wait(&sem);
    int result = _remove_block(get_file(file_db, get_file_name_from_block_name(block_name)), block_name, file_db->id);
    sem_post(&sem);
    return result;
}

int transfer_block(Grid_File_DB *file_db, char *block_name, u_int8_t new_dr_id) {
    sem_wait(&sem);
    Block_File *block = get_block(file_db, block_name);
    if (!block)
        return FALSE;
    block->dr_id = new_dr_id;
    sem_post(&sem);
    return TRUE;
}

// TODO test
void db_destroyer(Grid_File_DB *database) {

    Grid_File *temp = database->head;
    Grid_File *prec;
    while (temp->next) {
        prec = temp;
        temp = temp->next;
        remove_file(database, prec->name);
    }

    sem_destroy(&sem);
}