cmake_minimum_required(VERSION 3.7)
project(Grid_FTP C)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)
find_package(Doxygen)

include_directories(client)
include_directories(data_repository)
include_directories(metadata_server)
include_directories(utils)
include_directories(bin)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ./lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./bin)

add_executable(Grid_FTP_Server
        metadata_server/server.c
        metadata_server/server.h
        utils/dr_list.c
        utils/dr_list.h
        utils/colors.h
        utils/common.h
        utils/protocol.h utils/protocol.c utils/error_helper.h utils/common.c utils/file_db.c utils/file_db.h)

add_executable(Grid_FTP_Client
        client/client.c
        client/client.h
        utils/colors.h
        utils/common.h
        utils/dr_list.c
        utils/dr_list.h
        utils/protocol.h utils/protocol.c utils/error_helper.h utils/common.c utils/file_db.c utils/file_db.h)

add_executable(Grid_FTP_Repository
        data_repository/data_repository.c
        data_repository/data_repository.h
        utils/colors.h
        utils/common.h
        utils/dr_list.c
        utils/dr_list.h
        utils/protocol.h utils/protocol.c utils/error_helper.h utils/common.c utils/file_db.c utils/file_db.h)

target_link_libraries(Grid_FTP_Server Threads::Threads)
target_link_libraries(Grid_FTP_Repository Threads::Threads)
target_link_libraries(Grid_FTP_Client Threads::Threads)

target_link_libraries(Grid_FTP_Server crypt)
target_link_libraries(Grid_FTP_Repository crypt)
target_link_libraries(Grid_FTP_Client crypt)
