CC ?= $(CROSS_COMPILE)gcc
SRC := aesdsocket.c
TARGET ?= aesdsocket
OBJS := $(SRC:.c=.o)
CFLAGs ?= -g -Wall -Werror
LDFLAGS ?= -lpthread -lrt

all: $(TARGET)

default: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS) $(INCLUDES) $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) 
