PATH_CPP = jni
PATH_HEADERS = jni/includes

SRC_CPP = \
	$(PATH_CPP)/init.cpp \
	$(PATH_CPP)/socket.cpp \
	$(PATH_CPP)/utils.cpp \
	$(PATH_CPP)/main.cpp \
	$(PATH_CPP)/run_proxy.cpp

SRC_HEADERS = \
	$(PATH_HEADERS)/init.h \
	$(PATH_HEADERS)/socket.h \
	$(PATH_HEADERS)/utils.h \
	$(PATH_HEADERS)/main.h \
	$(PATH_HEADERS)/run_proxy.h

CC = g++
CFLAGS = -Wall -Wextra
LIB = -lpthread
EXE = linphone_proxy

build: all

all: $(EXE)

$(EXE): $(SRC_CPP) $(SRC_HEADERS)
	$(CC) $(CFLAGS) $(SRC_CPP) -o $@ $(LIB)

clean:
	rm -f $(EXE)
