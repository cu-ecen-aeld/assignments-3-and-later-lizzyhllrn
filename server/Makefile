CC ?= $(CROSS_COMPILE)gcc
SRC := aesdsocket-simple.c
TARGET ?= aesdsocket-simple
OBJS := $(SRC:.c=.o)
CFLAGs ?= -g -Wall -Werror
LDFLAGS ?= -lpthread -lrt

all: $(TARGET)

default: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS) $(INCLUDES) $(LDFLAGS)
	g++ -c capture.cpp -o capture.o
	gcc aesdsocket-simple.c capture.o aesdsocket-simple

clean:
	-rm -f *.o $(TARGET) 