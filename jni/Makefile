CC = gcc
CFLAGS = -Wall -Wextra -g
INCLUDE = -I includes

SRCS = \
    init.c \
    socket.c \
    utils.c \
    main.c \
    run_proxy.c

FINAL = linphone_proxy

all: build

build: $(FINAL)

$(FINAL): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

.PHONY: clean

clean:
	rm -f $(FINAL)

