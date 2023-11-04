CROSS_COMPILE ?=
SRC := aesdsocket-2.c
TARGET = aesdsocket-2
OBJS := $(SRC:.c=.o)
CC = $(CROSS_COMPILE)gcc
CFLAGs += -g -Wall -Werror

all: $(TARGET)

default: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)

clean:
	-rm -f *.o $(TARGET) 
