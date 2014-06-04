PATH_C = jni
PATH_HEADERS = jni/includes

SRC_C = \
	$(PATH_C)/main.c \
	$(PATH_C)/parse.c

SRC_HEADERS = \
	$(PATH_HEADERS)/socket.h \
	$(PATH_HEADERS)/parse.h \
	$(PATH_HEADERS)/utils.h

CC = gcc
CFLAGS = -Wall -Wextra -O3
EXE = linphone_proxy

build: all

all: $(EXE)

$(EXE): $(SRC_C) $(SRC_HEADERS)
	$(CC) $(CFLAGS) $(SRC_C) -o $@

clean:
	rm -f $(EXE)

