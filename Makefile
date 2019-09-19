CC = gcc
CFLAGS= #-Wall -Werror -Wextra -pedantic
OPT = -O0
LDFLAGS = -ggdb -lpthread
PFLAGS= -pg # used just for debug
CMN = utils/common.h
SERVER_CMN = metadata_server/dr_list.h metadata_server/dr_list.c

all: clean cartella server client_test data_rep

clean:
	rm -rf build
cartella:
	mkdir -p build
	cp utils/repositories.conf build/rep.conf
	cp utils/repositories.conf rep.conf # just for debug inside clion
server:
	$(CC) $(CFLAGS) $(OPT) $(CMN) $(SERVER_CMN) metadata_server/server.c -o build/server $(LDFLAGS)
client_test:
	$(CC) $(CFLAGS) $(OPT) $(CMN) client/client.c -o build/client $(LDFLAGS)
data_rep:
	$(CC) $(CFLAGS) $(OPT) $(CMN) data_repository/data_repository.c -o build/data_repository $(LDFLAGS)