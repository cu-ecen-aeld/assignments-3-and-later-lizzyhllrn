LIB_DIRS = 
CC 	:= $(CROSS_COMPILE)g++

CDEFS=
CFLAGS:= -O0 -g `pkg-config --cflags opencv4`

LIBS= -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_video -lopencv_videoio -lrt -lstdc++fs -lpthread -lstdc++

HFILES= 
CFILES= capture.cpp

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.cpp=.o}

all:	capture

clean:
	-rm -f *.o *.d
	-rm -f capture

capture: capture.cpp
	$(CC) capture.cpp $(CFLAGS) -o capture `pkg-config --cflags --libs opencv4` $(LIBS)

depend:

