#include "common.h"
#include "SHMRequestChannel.h"
#include <sys/mman.h>
using namespace std;

SHMRequestChannel::SHMRequestChannel(const string _name, const Side _side) : RequestChannel(_name, _side){

	sideNum = _side;
	
	shm1 = "/shm_" + my_name + "1";
	shm2 = "/shm_" + my_name + "2";

	queue1 = new SHMQueue(shm1);
	queue2 = new SHMQueue(shm2);
	
}

SHMRequestChannel::~SHMRequestChannel() {

	delete queue1;
	delete queue2;

}

int SHMRequestChannel::cwrite (void* msgbuf, int bufcapacity) {


	if (sideNum == SERVER_SIDE){

		sem_wait(queue1->readerdone);

		memcpy(queue1->shmbuffer, msgbuf, bufcapacity);

		sem_post(queue1->writerdone);
	}
	else{
		sem_wait(queue2->readerdone);
		memcpy(queue2->shmbuffer, msgbuf, bufcapacity);
		sem_post(queue2->writerdone);
	}

	
	return bufcapacity;

}


int SHMRequestChannel::cread (void *msgbuf , int msglen) {

	if (sideNum == SERVER_SIDE){


		sem_wait(queue2->writerdone);

		memcpy(msgbuf, queue2->shmbuffer, msglen);
		
		sem_post(queue2->readerdone);

	}
	else{

		sem_wait(queue1->writerdone);

		memcpy(msgbuf, queue1->shmbuffer, msglen);
		
		sem_post(queue1->readerdone);

	}


	return msglen;

}


SHMQueue::SHMQueue (string name) {

	QueueName = name;
	shm_fd = shm_open((char *) QueueName.c_str(), O_RDWR | O_CREAT, 0644);

	ftruncate(shm_fd, MAX_MESSAGE);

	shmbuffer = (char*) mmap(NULL, MAX_MESSAGE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	shmbuffer[0] = 0;
	
	sem1 = QueueName + "_sem1";
	sem2 = QueueName + "_sem2";

	readerdone = sem_open((char *) sem1.c_str(), O_CREAT, 0644, 1);
	writerdone = sem_open((char *) sem2.c_str(), O_CREAT, 0644, 0);
}	

SHMQueue::~SHMQueue() {

	close(shm_fd);
	munmap(shmbuffer, MAX_MESSAGE);
	shm_unlink(QueueName.c_str());

	sem_close(readerdone);
	sem_close(writerdone);
	sem_unlink(sem1.c_str());
	sem_unlink(sem2.c_str());
}
