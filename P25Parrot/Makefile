CC      = cc
CXX     = c++
CFLAGS  = -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lpthread
LDFLAGS = -g

OBJECTS = Network.o P25Parrot.o Parrot.o StopWatch.o Thread.o Timer.o UDPSocket.o

all:		P25Parrot

P25Parrot:	GitVersion.h $(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o P25Parrot

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

P25Parrot.o: GitVersion.h FORCE

.PHONY: GitVersion.h

FORCE:


install:
		install -m 755 P25Parrot /usr/local/bin/

clean:
		$(RM) P25Parrot *.o *.d *.bak *~

# Export the current git version if the index file exists, else 000...
GitVersion.h:
ifneq ("$(wildcard ../.git/index)","")
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@
else
	echo "const char *gitversion = \"0000000000000000000000000000000000000000\";" > $@
endif

