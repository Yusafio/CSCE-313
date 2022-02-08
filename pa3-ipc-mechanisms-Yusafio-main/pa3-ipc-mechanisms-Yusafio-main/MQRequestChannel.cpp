#include "common.h"
#include "MQRequestChannel.h"
using namespace std;

MQRequestChannel::MQRequestChannel(const string _name, const Side _side, long msgcap) : RequestChannel(_name, _side){

	mq1 = "/mq_" + my_name + "1";
	mq2 = "/mq_" + my_name + "2";


	struct mq_attr attributes;
	attributes.mq_flags = 0;
	attributes.mq_maxmsg = 1;
	attributes.mq_msgsize = msgcap;
	attributes.mq_curmsgs = 0;

	if (_side == SERVER_SIDE){
		wfd = mq_open( (char *) mq1.c_str(), O_CREAT | O_WRONLY, 0644, &attributes);
		rfd = mq_open( (char *) mq2.c_str(), O_CREAT | O_RDONLY, 0644, &attributes);
	}
	else{
		rfd = mq_open((char *) mq1.c_str(), O_CREAT | O_RDONLY, 0644, &attributes);
		wfd = mq_open((char *) mq2.c_str(), O_CREAT | O_WRONLY, 0644, &attributes);
	}
	
}

MQRequestChannel::~MQRequestChannel() {

	mq_close(wfd);
	mq_close(rfd);

	mq_unlink(mq1.c_str());
	mq_unlink(mq2.c_str());
}

int MQRequestChannel::cwrite(void * msgbuf, int msglen) {


	return mq_send(wfd, (char *) msgbuf, msglen, 0);
}


int MQRequestChannel::cread(void * msgbuf , int msglen) {


	return mq_receive(rfd, (char *) msgbuf, 4000, NULL);

}