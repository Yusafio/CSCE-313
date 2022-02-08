
#ifndef _MQRequestChannel_H_
#define _MQRequestChannel_H_

#include "common.h"
#include "RequestChannel.h"
#include <mqueue.h>
class MQRequestChannel: public RequestChannel {

private:
	
	string my_name;
	Side my_side;

    string mq1, mq2;

    mqd_t wfd;
	mqd_t rfd;

public:
        
    MQRequestChannel(const string _name, const Side _side, long msgcap);
        

    ~MQRequestChannel();

    int cwrite (void * msgbuf, int msglen);
    int cread (void * msgbuf , int msglen);

};

#endif