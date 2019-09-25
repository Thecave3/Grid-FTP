//
// Created by Andrea Lacava on 18/09/19.
//

#ifndef GRID_FTP_SERVER_H
#define GRID_FTP_SERVER_H

#include "../utils/common.h"

#define DATAREP_FILE_PATH "./rep.conf"
#define USERLIST_FILE_PATH "./user.db"

#define CNFG_FILE_DELIMITER ","

#define SERVER_PORT 3000

DR_List *get_data_repositories_info();

void server_routine();


#endif // GRID_FTP_SERVER_H
