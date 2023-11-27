LIB_DIRS = 
CROSS_COMPILE =
ifeq ($(CC),)
	CC = $(CROSS_COMPILE)g++
endif
ifeq ($(INCLUDE_DIRS),)
	INCLUDE_DIRS = -I/usr/local/include/opencv4
endif
#CC 	:= $(CROSS_COMPILE)g++
ifeq ($(CFLAGS),)
	CFLAGS:= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
endif
CDEFS=

LIBS= -L/usr/lib -lopencv_core -lopencv_flann -lopencv_video -lrt -lstdc++fs -lpthread -lstdc++
#LIBS=
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