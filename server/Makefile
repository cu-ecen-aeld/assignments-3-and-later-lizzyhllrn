# Compiler settings
CC := gcc
CXX := g++

# Compiler flags
CFLAGS := -g -Wall -Wextra
CXXFLAGS := -O0 -g -Wall -Wextra `pkg-config --cflags --libs opencv4`
LIBS= -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_video -lopencv_videoio -lrt -lstdc++fs -lpthread -lstdc++

# Source files
C_SRCS := aesdsocket-simple.c
CXX_SRCS := capture.cpp

# Object files
C_OBJS := $(C_SRCS:.c=.o)
CXX_OBJS := $(CXX_SRCS:.cpp=.o)

# Output binary
TARGET := aesdsocket-simple

# Cross-compilation settings
CROSS_COMPILE ?=

ifeq ($(CROSS_COMPILE),)
    # Native compilation
else
    # Cross-compilation settings
    CC := $(CROSS_COMPILE)$(CC)
    CXX := $(CROSS_COMPILE)$(CXX)
endif

all: $(TARGET)

capture.o: capture.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LIBS)

aesdsocket.o: aesdsocket-simple.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): capture.o aesdsocket.o
	$(CXX) $^ -o $@ $(LIBS)


.PHONY: clean

clean:
	rm -f $(C_OBJS) $(CXX_OBJS) $(TARGET)