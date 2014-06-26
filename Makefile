SRC_PATH=./jni/
INCL_PATH=./jni/includes/
FLAGS=-Wall -I$(INCL_PATH)

build: all

all: session_proxy

session_proxy: $(SRC_PATH)init.c $(INCL_PATH)init.h \
				$(SRC_PATH)socket.c $(INCL_PATH)socket.h \
				$(SRC_PATH)utils.c $(INCL_PATH)utils.h \
				$(SRC_PATH)main.c $(INCL_PATH)main.h \
				$(SRC_PATH)run_proxy.c $(INCL_PATH)run_proxy.h
	gcc $(SRC_PATH)init.c $(SRC_PATH)socket.c $(SRC_PATH)utils.c $(SRC_PATH)main.c $(SRC_PATH)run_proxy.c -o session_proxy $(FLAGS)

clean:
	rm -f ./session_proxy
