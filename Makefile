SRC_PATH=./jni/
FLAGS=-Wall

build: all

all: linphone_proxy

linphone_proxy: $(SRC_PATH)init.cpp \
				$(SRC_PATH)socket.cpp \
				$(SRC_PATH)utils.cpp \
				$(SRC_PATH)main.cpp \
				$(SRC_PATH)run_proxy.cpp
	g++ $(SRC_PATH)init.cpp $(SRC_PATH)socket.cpp $(SRC_PATH)utils.cpp $(SRC_PATH)main.cpp $(SRC_PATH)run_proxy.cpp -o linphone_proxy $(FLAGS)

clean:
	rm -f ./linphone_proxy