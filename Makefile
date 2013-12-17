SRC_PATH=./jni/
FLAGS=-Wall

build: all

all: linphone_proxy

linphone_proxy: $(SRC_PATH)init.c \
				$(SRC_PATH)socket.c \
				$(SRC_PATH)utils.c \
				$(SRC_PATH)main.c \
				$(SRC_PATH)run_proxy.c
	gcc $(SRC_PATH)init.c $(SRC_PATH)socket.c $(SRC_PATH)utils.c $(SRC_PATH)main.c $(SRC_PATH)run_proxy.c -o linphone_proxy $(FLAGS)

clean:
	rm -f ./linphone_proxy