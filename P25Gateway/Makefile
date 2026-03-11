CC      = cc
CXX     = c++

CFLAGS  = -g -O3 -Wall -std=c++0x -MMD -MD -pthread
LIBS    = -lpthread -lmosquitto

LDFLAGS = -g

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all:		P25Gateway

P25Gateway:	GitVersion.h $(OBJS)
		$(CXX) $(OBJS) $(CFLAGS) $(LIBS) -o P25Gateway

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<
-include $(DEPS)

P25Gateway.o: GitVersion.h FORCE

.PHONY: GitVersion.h

FORCE:

install:
		install -m 755 P25Gateway /usr/local/bin/

clean:
		$(RM) P25Gateway *.o *.d *.bak *~

# Export the current git version if the index file exists, else 000...
GitVersion.h:
ifneq ("$(wildcard ../.git/index)","")
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@
else
	echo "const char *gitversion = \"0000000000000000000000000000000000000000\";" > $@
endif

