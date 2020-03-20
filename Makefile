CC = g++
CXXFLAGS = -std=c++11
LFLAGS = -std=c++11
ALL_EXECS = process stopall # test_server test_client

PROCESS_OBJS = process.o p2p.o
STOPALL_OBJS = stopall.o p2p.o

all: $(ALL_EXECS)

clean:
	rm -f *.o $(ALL_EXECS)

process: $(PROCESS_OBJS)
	$(CC) $(LFLAGS) -o $@ $(PROCESS_OBJS)

stopall: $(STOPALL_OBJS)
	$(CC) $(LFLAGS) -o $@ $(STOPALL_OBJS)

# test_server: test_server.o p2p.o
# test_client: test_client.o p2p.o

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $<