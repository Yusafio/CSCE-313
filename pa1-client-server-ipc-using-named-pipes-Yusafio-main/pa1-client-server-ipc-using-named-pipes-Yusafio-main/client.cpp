/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
	Original author of the starter code
	
	Please include your name and UIN below
	Name: Yusuf Barati
	UIN: 528003435
 */
#include "common.h"
#include "FIFOreqchannel.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <sys/wait.h>
#include <chrono>

using namespace std;


int main(int argc, char *argv[]){

	//task 4 running server as child process
	pid_t process;

	process = fork();

	if (process < 0) {
		perror("Fork failed!");
		return 2;
	}

	if (process == 0) {

		argv[0] = "./server";
		execvp(argv[0], argv);

		perror("Execv failed!");
		return 2;
	}

	if (process > 0) {



    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
	
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool channelTwo = false;
	int buffercapacity = MAX_MESSAGE;
	
	// while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) 

	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:c:m:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'c':
				channelTwo = atoi (optarg);
				break;
			case 'm':
				buffercapacity = atoi (optarg);
				break;
		}
	}

	// task 1 collect 1000 datapts in a file
	// condition is negative time

	if (t < 0)
	{
		//open file
		std::ofstream taskOneFile;
		taskOneFile.open("received/x1.csv");
		
		//set t to 0, and increment for 1000 data points, p stays same
		char buf [MAX_MESSAGE]; // 256

		for (t = 0; t <= 40; t += .04)
		{
			taskOneFile << t << ",";
			e = 1;
			datamsg e1 (p, t, e);
			chan.cwrite (&e1, sizeof (datamsg)); //ask ecg1
			double reply1;
			chan.cread (&reply1, sizeof(double)); //reply
			taskOneFile << reply1 << ",";
			
			e = 2;
			datamsg e2 (p, t, e);
			chan.cwrite (&e2, sizeof (datamsg)); //ask ecg2
			double reply2;
			chan.cread (&reply2, sizeof(double)); //reply
			taskOneFile << reply2 << "\n";

		}
		taskOneFile.close();
	}

	// task 2 requesting a file
	// condition is filename != ""

	if (filename != "")
	{
		//open file

		string outpath = "received/" + filename;

		FILE* outfile = fopen (outpath.c_str(), "wb");

		//get content of file

		filemsg request(0,0);


		int len = sizeof (filemsg) + filename.size()+1;
		char buf2 [len];
		memset(buf2, 0, sizeof(buf2));
		memcpy (buf2, &request, sizeof (filemsg));
		strcpy (buf2 + sizeof (filemsg), filename.c_str());

		chan.cwrite (buf2, len);  // I want the file length;
		__int64_t fileSize;
		chan.cread(&fileSize, sizeof(__int64_t));

		char buf3 [buffercapacity];
		memset(buf3, 0, sizeof(buf3));

		request.offset = 0;
		request.length = buffercapacity;

		//timer start

		auto start = std::chrono::high_resolution_clock::now();
		
		for (__int64_t i = 0; i < fileSize - buffercapacity; i+= buffercapacity) {


			memcpy (buf2, &request, sizeof (filemsg));
			strcpy (buf2 + sizeof (filemsg), filename.c_str());
			chan.cwrite(&buf2, len);

			memset(buf3, 0, sizeof(buf3));
			chan.cread(&buf3, sizeof(buf3));
			//write to file
			fwrite(buf3, 1, buffercapacity, outfile);

			request.offset += buffercapacity;

		}



		//write remainder
		request.length = fileSize % buffercapacity;

		//if 0, means was perfect multiple of buffercapacity, and buffercapacity bytes are still left
		if (request.length == 0) {
			request.length = buffercapacity;
		}

		memcpy (buf2, &request, sizeof (filemsg));
		strcpy (buf2 + sizeof (filemsg), filename.c_str());
		chan.cwrite(&buf2, len);

		memset(buf3, 0, sizeof(buf3));
		chan.cread(&buf3, sizeof(buf3));

		fwrite(buf3, 1, request.length, outfile);

		//timer end

		auto stop = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
		cout << "File transfer time: " << duration.count() << endl;

	}

	//task 3 new channel

	if(channelTwo == true) {

		cout << "reached channelTwo" << endl;

		MESSAGE_TYPE cm = NEWCHANNEL_MSG;			//create new channel with -c arg
    	chan.cwrite (&cm, sizeof (MESSAGE_TYPE));
		string newChannelName;

		char c2buf [30];
		memset(c2buf, 0, sizeof(c2buf));

		chan.cread(&c2buf, sizeof(c2buf));
		FIFORequestChannel chan2 (c2buf, FIFORequestChannel::CLIENT_SIDE);


		char c2buf2 [MAX_MESSAGE]; // 256
		datamsg c2m (p, t, e);
		
		chan2.cwrite (&c2m, sizeof (datamsg)); // question
		double c2reply;
		chan2.cread (&c2reply, sizeof(double)); //answer
		cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << c2reply << endl;


		// closing the channel    
		MESSAGE_TYPE qm2 = QUIT_MSG;
		chan2.cwrite (&qm2, sizeof (MESSAGE_TYPE));

	}
	
    // sending a non-sense message, you need to change this
    char buf [MAX_MESSAGE]; // 256
    datamsg x (p, t, e);
	
	chan.cwrite (&x, sizeof (datamsg)); // question
	double reply;
	int nbytes = chan.cread (&reply, sizeof(double)); //answer
	cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;
	
	filemsg fm (0,0);
	string fname = "teslkansdlkjflasjdf.dat";
	
	int len = sizeof (filemsg) + fname.size()+1;
	char buf2 [len];
	memcpy (buf2, &fm, sizeof (filemsg));
	strcpy (buf2 + sizeof (filemsg), fname.c_str());
	chan.cwrite (buf2, len);  // I want the file length;

	
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));

	}


	int status;
	pid_t wait_result;

	while ( (wait_result  = wait(&status)) != -1) {
		printf("Process %lu returned result: %d\n", (unsigned long) wait_result, status);
	}

	printf("All children have finished.\n");

	return 0;

}
