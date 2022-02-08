#ifndef _SHMRequestChannel_H_
#define _SHMRequestChannel_H_

#include "common.h"
#include "RequestChannel.h"
#include <semaphore.h>

class SHMQueue {
public:
    string sem1;
    string sem2;
    sem_t * readerdone;
    sem_t * writerdone;

    char* shmbuffer;
    int shm_fd;
    string QueueName;

    SHMQueue (string name);
    ~SHMQueue();
};

class SHMRequestChannel: public RequestChannel {

private:
	
    int sideNum;
	string my_name;
	Side my_side;

    string shm1, shm2;
    SHMQueue* queue1;
    SHMQueue* queue2;

public:
        
    SHMRequestChannel(const string _name, const Side _side);
	
    ~SHMRequestChannel();

    int cwrite (void* msgbuf, int bufcapacity);
    int cread (void *msgbuf , int msglen);

};

#endif
