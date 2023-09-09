SRC := writer.c
TARGET = writer
OBJS := $(SRC:.c=.o)
CC = $(CROSS_COMPILE)gcc


all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) 

clean:
	-rm -f *.o $(TARGET) 
