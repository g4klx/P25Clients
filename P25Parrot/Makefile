CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lpthread
LDFLAGS = -g

OBJECTS = Network.o P25Parrot.o Parrot.o StopWatch.o Thread.o Timer.o UDPSocket.o

all:		P25Parrot

P25Parrot:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o P25Parrot

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) P25Parrot *.o *.d *.bak *~
