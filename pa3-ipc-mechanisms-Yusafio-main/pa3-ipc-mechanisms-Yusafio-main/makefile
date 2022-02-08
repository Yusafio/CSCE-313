# makefile

all: server client

common.o: common.h common.cpp
	g++ -g -w -std=c++11 -c common.cpp

FIFOreqchannel.o: RequestChannel.h FIFOreqchannel.h FIFOreqchannel.cpp
	g++ -g -w -std=c++11 -c FIFOreqchannel.cpp

MQRequestChannel.o: RequestChannel.h MQRequestChannel.h MQRequestChannel.cpp -lrt
	g++ -g -w -std=c++11 -c MQRequestChannel.cpp

SHMRequestChannel.o: RequestChannel.h SHMRequestChannel.h SHMRequestChannel.cpp -lrt
	g++ -g -w -std=c++11 -c SHMRequestChannel.cpp

client: client.cpp FIFOreqchannel.o common.o MQRequestChannel.o SHMRequestChannel.o
	g++ -g -w -std=c++11 -o client client.cpp FIFOreqchannel.o common.o MQRequestChannel.o SHMRequestChannel.o -lpthread -lrt

server: server.cpp  FIFOreqchannel.o common.o MQRequestChannel.o SHMRequestChannel.o
	g++ -g -w -std=c++11 -o server server.cpp FIFOreqchannel.o common.o MQRequestChannel.o SHMRequestChannel.o -lpthread -lrt

clean:
	rm -rf *.o *.csv fifo* server client data*_*
