LIB_DIRS = 
CC 	:= $(CROSS_COMPILE)g++

CDEFS=
CFLAGS:= -O0 -g `pkg-config --cflags opencv4`

LIBS= -lopencv_core -lopencv_tracking -lopencv_flann -lopencv_video -lrt -lstdc++fs -lpthread -lstdc++

HFILES= 
CFILES= capture.cpp

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.cpp=.o}

all:	capture

clean:
	-rm -f *.o *.d
	-rm -f capture

capture: capture.cpp
	$(CC) capture.cpp $(CFLAGS) -o capture.o `pkg-config --cflags --libs opencv4` $(LIBS)

depend:

