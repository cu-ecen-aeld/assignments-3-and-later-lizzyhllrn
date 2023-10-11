SRC := aesdsocket.c
TARGET = aesdsocket
OBJS := $(SRC:.c=.o)
CC = $(CROSS_COMPILE)gcc
CFLAGs += -g -Wall -Werror

all: $(TARGET)

default: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)

clean:
	-rm -f *.o $(TARGET) 
