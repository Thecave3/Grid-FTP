//
// Created by Andrea Lacava on 26/09/19.
//

#include <stdio.h>
#include <sys/param.h>
#include <stdlib.h>
#include "error_helper.h"

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
    int is_dr;
    u_int8_t id;
} Grid_File_DB;


Grid_File_DB *init_db(int is_dr, u_int8_t id);

void file_db_to_string(Grid_File_DB *database, char *buffer);

Grid_File *get_file(Grid_File_DB *database, char *name);

Grid_File *new_file(char *name, unsigned long size, Block_File *head_block);

int add_file(Grid_File_DB *database, char *name, long unsigned size, Block_File *list);

int remove_file(Grid_File_DB *database, char *name);

char *file_to_string(Grid_File *file);

char *get_file_name_from_block_name(char *block_name);

void block_to_string(char *result, Block_File *block);

Block_File *new_block(char *block_name, u_int8_t dr_id, unsigned long start, unsigned long end);

Block_File *append_block(Block_File *head, Block_File *block_new);

Block_File *get_block(Grid_File_DB *file_db, char *block_name);

int move_block(Grid_File_DB *file_db, char *block_name, u_int8_t new_dr_id);

int remove_block(Grid_File_DB *file_db, char *block_name);

void db_destroyer(Grid_File_DB* file_db);

int transfer_block(Grid_File_DB *file_db, char *block_name, u_int8_t new_dr_id);

void update_file_db_from_string(Grid_File_DB *database, char *buffer);

#endif //GRID_FTP_FILE_DB_H
