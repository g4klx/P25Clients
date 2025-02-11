CC      = cc
CXX     = c++
CFLAGS  = -g -O3 -Wall -DHAVE_LOG_H -std=c++0x -pthread
LIBS    = -lpthread
LDFLAGS = -g

OBJECTS = Conf.o DMRLookup.o Log.o Mutex.o P25Gateway.o P25Network.o Reflectors.o RptNetwork.o StopWatch.o Thread.o Timer.o UDPSocket.o Utils.o Voice.o

all:		P25Gateway

P25Gateway:	GitVersion.h $(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o P25Gateway

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

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

