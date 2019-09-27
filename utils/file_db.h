//
// Created by Andrea Lacava on 26/09/19.
//

#include <sys/param.h>

#ifndef GRID_FTP_FILE_DB_H
#define GRID_FTP_FILE_DB_H

typedef struct Block_File {
    char *block_name;
    u_int8_t dr_id;
    unsigned long start;
    unsigned long end;
    struct Block_File *next;
} Block_File;


typedef struct Grid_File {
    long unsigned size;
    char *name;
    Block_File *head;
    struct Grid_File *next;
} Grid_File;

typedef struct Grid_File_DB {
    Grid_File *head;
} Grid_File_DB;


Grid_File_DB *init_db();

Grid_File *get_file(Grid_File_DB *database, char *name);

int add_file(Grid_File_DB *database, char *name, long unsigned size, Block_File *list);

int remove_file(Grid_File_DB *database, char *name);

char *file_to_string(Grid_File *file);

Block_File *new_block(u_int8_t dr_id, unsigned long start, unsigned long end);

void add_block(Block_File *head, Block_File *block);

Block_File *get_block(Grid_File_DB *file_db, char *block_name);

int move_block(Grid_File_DB *file_db, char *block_name, u_int8_t new_dr_id);

int remove_block(Grid_File_DB *file_db, char *block_name);

int transfer_block(Grid_File_DB *file_db, char *block_name, char *new_dr_id);

#endif //GRID_FTP_FILE_DB_H
