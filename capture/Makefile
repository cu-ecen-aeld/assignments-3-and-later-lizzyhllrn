INCLUDE_DIRS = -I/usr/include/opencv4
LIB_DIRS = 
CC 	:= $(CROSS_COMPILE)g++

CDEFS=
CFLAGS:= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lopencv_core -lopencv_flann -lopencv_video -lrt -lstdc++fs -lpthread -lstdc++

HFILES= 
CFILES= capture.cpp

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.cpp=.o}

all:	capture

clean:
	-rm -f *.o *.d
	-rm -f capture

capture: capture.cpp
	$(CC) capture.cpp $(CFLAGS) -o $@ `pkg-config --libs opencv4` $(LIBS)

depend:

.cpp.o: $(SRCS)
	$(CC) $(CFLAGS) -c $<