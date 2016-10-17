CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lpthread
LDFLAGS = -g

OBJECTS = Conf.o DMRLookup.o Log.o Mutex.o Network.o P25Gateway.o Reflectors.o Speech.o StopWatch.o Thread.o Timer.o UDPSocket.o Utils.o

all:		P25Gateway

P25Gateway:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o P25Gateway

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) P25Gateway *.o *.d *.bak *~
 