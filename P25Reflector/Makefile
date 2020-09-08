CC      = cc
CXX     = c++
CFLAGS  = -g -O3 -Wall -DHAVE_LOG_H -DUDP_SOCKET_MAX=2 -std=c++0x -pthread
LIBS    = -lpthread
LDFLAGS = -g

OBJECTS = Conf.o DMRLookup.o Log.o Mutex.o Network.o P25Reflector.o StopWatch.o Thread.o Timer.o UDPSocket.o Utils.o

all:		P25Reflector

P25Reflector:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o P25Reflector

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 P25Reflector /usr/local/bin/

clean:
		$(RM) P25Reflector *.o *.d *.bak *~
 
