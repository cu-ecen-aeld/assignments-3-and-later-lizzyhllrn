# the ?= is not compatible in all the make versions so making it cross compatible by using := instead
INCLUDE_DIRS =
LIB_DIRS = 
CC 	:= $(CROSS_COMPILE)g++

CDEFS=
CFLAGS:= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lopencv_core -lopencv_flann -lopencv_video -lrt -lstdc++fs -lpthread -lstdc++

HFILES= 
CFILES= aesd_opencv.cpp

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.cpp=.o}

all:	aesd_opencv

clean:
	-rm -f *.o *.d
	-rm -f aesd_opencv

aesd_opencv: aesd_opencv.cpp
	$(CC) aesd_opencv.cpp $(CFLAGS) -o $@ `pkg-config --libs opencv4` $(LIBS)

depend:

.cpp.o: $(SRCS)
	$(CC) $(CFLAGS) -c $<