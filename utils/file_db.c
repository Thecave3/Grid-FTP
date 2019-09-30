//
// Created by Andrea Lacava on 26/09/19.
//

#include "file_db.h"
#include "common.h"
#include "protocol.h"
#include <semaphore.h>

#define CRITICAL_SECTION_SIZE 1
sem_t sem;

/**
 * Initialization of database and semaphore to handle concurrency
 *
 * @param is_dr if the caller is a dr or the server
 * @param id of the caller passed by user (0 if server)
 * @return database initialized and ready to be used
 */
Grid_File_DB *init_db(int is_dr, u_int8_t id) {
    Grid_File_DB *database = (Grid_File_DB *) malloc(sizeof(Grid_File_DB));
    database->is_dr = is_dr;
    database->id = id;
    database->head = NULL;
    sem_init(&sem, 0, CRITICAL_SECTION_SIZE);
    return database;
}

/**
 * ToString function of the entire database, based on single function.
 * Values are concateneted to the buffer by using strncat();.
 *
 * @param database to be stringed
 * @param buffer in which the database will be stored.
 */
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

/**
 *  Test if the the strings are one the block of the other file.
 *
 * @param filename name of the file
 * @param blockname name of the block
 * @return  TRUE if they are, FALSE otherwise
 */
int is_a_block_of_file(char *filename, char *blockname) {
    if (!filename || !blockname)
        return FALSE;
    char *block_radix = strtok(blockname, FILE_BLOCK_SEPARATOR);
    return strncmp(block_radix, filename, strlen(filename));
}

/**
 *  Parse the stringed database and updated the database passed in.
 *
 * @param database to be updated
 * @param buffer string with the following format <F,name,size,B,block_name,dr_id,start,end>
 *
 */
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

/**
 * Compare if the file is the same of the one passed with name.
 *
 * @param file struct of the file
 * @param n name of the file
 * @return TRUE if the file match with n, FALSE otherwise
 */
static int file_compare(Grid_File *file, char *n) {
    size_t fstr_size = strlen(file->name);
    size_t n_size = strlen(n);
    if (fstr_size != n_size)
        return FALSE;
    if (strncmp(file->name, n, fstr_size) == 0)
        return TRUE;
    return FALSE;
}

/**
 * Compare if the block is the same of the one passed with name.
 *
 * @param block struct of the file
 * @param n name of the file
 * @return TRUE if the file match with n, FALSE otherwise
 */
static int block_compare(Block_File *block, char *n) {
    size_t bstr_size = strlen(block->block_name);
    size_t n_size = strlen(n);
    if (bstr_size != n_size)
        return FALSE;
    if (strncmp(block->block_name, n, bstr_size) == 0)
        return TRUE;
    return FALSE;
}


/**
 * Static research of a file by using the name in the database.
 *
 * @param db database of the file
 * @param name of the file to be searched
 * @return TRUE if the file is present, FALSE otherwise
 */
static u_int8_t find_file(Grid_File_DB *db, char *name) {
    for (Grid_File *file = db->head; file; file = file->next)
        if (file_compare(file, name))
            return TRUE;
    return FALSE;
}

/**
 *  Get the tail of the database.
 *  Just an aux function to write more clean code.
 *
 * @param db database of files
 * @return tail of the database
 */
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

/**
 *  Get a file from a specific name in the database
 *
 * @param database to search in
 * @param name of the file requested
 * @return the pointer to the file if it is present, NULL otherwise.
 */
Grid_File *get_file(Grid_File_DB *database, char *name) {
    sem_wait(&sem);
    Grid_File *result = _get_file(database->head, name);
    sem_post(&sem);
    return result;
}

/**
 * Constructor for the file
 *
 * @param name of the file
 * @param size of the file
 * @param head_block first block on the file if present
 * @return the struct file
 */
Grid_File *new_file(char *name, unsigned long size, Block_File *head_block) {
    Grid_File *file = (Grid_File *) malloc(sizeof(Grid_File));
    file->name = name;
    file->head = head_block;
    file->size = size;
    file->next = NULL;
    return file;
}

/**
 * Creates a new file and add it the the tail of the database.
 *
 * @param database where the file has to be put in
 * @param name of the new file
 * @param size of the new file
 * @param head of the new file
 * @return TRUE if the file is added, if the file was already present FALSE
 */
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

/**
 * Remove a file from the database.
 * If the caller is a dr and the file is in this dr the file is also remove physically from the dr.
 *
 * @param database
 * @param name
 * @return TRUE if the file has been removed, FALSE otherwise
 */
int remove_file(Grid_File_DB *database, char *name) {

    sem_wait(&sem);
    int result = _remove_file(database->head, name, database->is_dr, database->id);
    sem_post(&sem);
    return result;
}

/**
 * Auxiliary function to get the file name from its sub blocks.
 *
 * It duplicates the string, splits the string at the separator in order to extract the file_name.
 * Output string is dynamically allocated.
 * This string since is created with strndup(); it has to be free by the caller after its use.
 *
 * @param block_name
 * @return the filename extracted
 */

char *get_file_name_from_block_name(char *block_name) {

    char *b_str = strndup(block_name, strlen(block_name));
    char *f_str = strtok(b_str, FILE_BLOCK_SEPARATOR);
    return f_str;
}

/**
 * Just a fast way to print to a buffer the block,
 *
 * @param result buffer
 * @param block to be printed with format <B,block_name,dr_id,start,end>
 */
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

/**
* Just a fast way to string a file,
*
* @param file struct of the file to be stringed
* @return block to be printed with format <F,name,size,<B,block_name,dr_id,start,end>>
*/
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

/**
 * Constructor for a new block
 *
 * @param block_name name of the block
 * @param dr_id of the repository where the block should be
 * @param start offset
 * @param end offset
 * @return the struct created and allocated
 */
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

/**
 * Append to the tail of the block list the block_new.
 *
 * @param head of the list
 * @param block_new to be put
 * @return the head of the block list
 */
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

/**
 * Get the block from the database
 *
 * @param file_db
 * @param block_name
 * @return the block if exists, NULL otherwise
 */
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

/**
 * remove a block from the database.
 *
 * @param file_db db because C does not support classes.
 * @param block_name name of the block because it is primary key
 * @return TRUE if the block has been removed, FALSE otherwise
 */
int remove_block(Grid_File_DB *file_db, char *block_name) {
    sem_wait(&sem);
    int result = _remove_block(get_file(file_db, get_file_name_from_block_name(block_name)), block_name, file_db->id);
    sem_post(&sem);
    return result;
}

/**
 * Update the position of the block in the system.
 *
 * @param file_db database to update
 * @param block_name name of the block
 * @param new_dr_id new location
 * @return TRUE if the block was found and updated, FALSE otherwise
 */
int transfer_block(Grid_File_DB *file_db, char *block_name, u_int8_t new_dr_id) {
    sem_wait(&sem);
    Block_File *block = get_block(file_db, block_name);
    if (!block)
        return FALSE;
    block->dr_id = new_dr_id;
    sem_post(&sem);
    return TRUE;
}


/**
 * Destroy the database and free the semaphore.
 *
 * @param database
 */
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