//
// Created by Andrea Lacava on 18/09/19.
//

#ifndef GRID_FTP_SERVER_H
#define GRID_FTP_SERVER_H

#include "../utils/common.h"
#include "dr_list.h"

#define DATAREP_FILE_PATH "./rep.conf"
#define CNFG_FILE_DEL ","

DR_List *get_data_repositories_info();

#endif // GRID_FTP_SERVER_H
