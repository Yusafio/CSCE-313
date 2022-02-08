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
#include "MQRequestChannel.h"
#include "SHMRequestChannel.h"

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
	
	int opt;
	int p = 1;
	double t = -1.0;
	int e = 1;
	int numChannels = 0;
	int buffercapacity = MAX_MESSAGE;
	
	string channelType = "f";
	auto start = std::chrono::high_resolution_clock::now();


	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:c:m:i:")) != -1) {
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
				numChannels = atoi (optarg);
				break;
			case 'm':
				buffercapacity = atoi (optarg);
				break;
			case 'i':
				channelType = optarg;
				break;
		}
	}

	RequestChannel* chan;
	if (channelType == "f") {
		chan = new FIFORequestChannel("control", RequestChannel::CLIENT_SIDE);
	}
	else if (channelType == "q") {
		chan = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE, buffercapacity);
	}
	else if (channelType == "s") {
		chan = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE);
	}


	int fileTransferTracker = 0;
	if(numChannels > 0) {

		for (int i = 0; i < numChannels; i ++) {

			cout << "entering channel " << (i + 1) << endl;

			sleep(1);

			MESSAGE_TYPE cm = NEWCHANNEL_MSG;			//create new channel with -c arg
			chan->cwrite (&cm, sizeof (MESSAGE_TYPE));

			char channelNameBuf [30];
			memset(channelNameBuf, 0, sizeof(channelNameBuf));

			chan->cread(&channelNameBuf, sizeof(channelNameBuf));

			RequestChannel* newChannel;
			cout << channelNameBuf << endl;

			if (channelType == "f") {
				newChannel = new FIFORequestChannel(channelNameBuf, RequestChannel::CLIENT_SIDE);
			}
			else if (channelType == "q") {
				newChannel = new MQRequestChannel(channelNameBuf, RequestChannel::CLIENT_SIDE, buffercapacity);
			}
			else if (channelType == "s") {
				newChannel = new SHMRequestChannel(channelNameBuf, RequestChannel::CLIENT_SIDE);
			}


			if (filename != "")
			{

				
				//open file

				string outpath = "received/" + filename;

				FILE* outfile;
				if (fileTransferTracker == 0) {
					outfile = fopen (outpath.c_str(), "wb");
				}
				else {
					outfile = fopen (outpath.c_str(), "ab");
				}


				
				filemsg request(0,0);

				int len = sizeof (filemsg) + filename.size()+1;
				char fileRequestBuf [len];
				memset(fileRequestBuf, 0, sizeof(fileRequestBuf));
				memcpy (fileRequestBuf, &request, sizeof (filemsg));
				strcpy (fileRequestBuf + sizeof (filemsg), filename.c_str());

				newChannel->cwrite (fileRequestBuf, len);  // I want the file length;
				__int64_t fileSize = 0;
				newChannel->cread(&fileSize, sizeof(__int64_t));

				char readBuf [buffercapacity];
				memset(readBuf, 0, sizeof(readBuf));

				request.offset = fileTransferTracker * (fileSize/numChannels);
				request.length = buffercapacity;

				//timer start

				// cout << "reading from " << fileTransferTracker * (fileSize/numChannels) << " to " << ((1 + fileTransferTracker)*(fileSize/numChannels)) << endl;
				
				for (__int64_t j = fileTransferTracker * (fileSize/numChannels); j < ((1 + fileTransferTracker)*(fileSize/numChannels)) - buffercapacity; j+= buffercapacity) {
					
					memset(fileRequestBuf, 0, sizeof(fileRequestBuf));
					memcpy (fileRequestBuf, &request, sizeof (filemsg));
					strcpy (fileRequestBuf + sizeof (filemsg), filename.c_str());
					newChannel->cwrite(&fileRequestBuf, len);

					memset(readBuf, 0, sizeof(readBuf));
					newChannel->cread(&readBuf, sizeof(readBuf));
					//write to file
					fwrite(readBuf, 1, buffercapacity, outfile);

					request.offset += buffercapacity;

				}

				fileTransferTracker++;


				//write remainder
				request.length = (fileSize/numChannels) % buffercapacity;

				//if 0, means was perfect multiple of buffercapacity, and buffercapacity bytes are still left
				if (request.length == 0) {
					request.length = buffercapacity;
				}

				memset(fileRequestBuf, 0, sizeof(fileRequestBuf));
				memcpy (fileRequestBuf, &request, sizeof (filemsg));
				strcpy (fileRequestBuf + sizeof (filemsg), filename.c_str());
				newChannel->cwrite(&fileRequestBuf, len);

				memset(readBuf, 0, sizeof(readBuf));
				newChannel->cread(&readBuf, sizeof(readBuf));

				fwrite(readBuf, 1, request.length, outfile);


				request.offset += request.length;

				if (fileTransferTracker == numChannels) {
					if(request.offset != fileSize) {
						request.length = fileSize - request.offset;

						memset(fileRequestBuf, 0, sizeof(fileRequestBuf));
						memcpy (fileRequestBuf, &request, sizeof (filemsg));
						strcpy (fileRequestBuf + sizeof (filemsg), filename.c_str());
						newChannel->cwrite(&fileRequestBuf, len);

						memset(readBuf, 0, sizeof(readBuf));
						newChannel->cread(&readBuf, sizeof(readBuf));

						fwrite(readBuf, 1, request.length, outfile);
					}
				}

				//timer end
				if (fileTransferTracker == numChannels) {
					auto stop = std::chrono::high_resolution_clock::now();

					auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
					cout << "File transfer time: " << duration.count() << endl;
				}

				fclose(outfile);

			}
			//collect 1000
			else if (t < 0) {

				cout << "entered 1000" << endl;
				//open file
				std::ofstream taskOneFile;
				taskOneFile.open("received/x1.csv");
				
				//set t to 0, and increment for 1000 data points, p stays same
				for (double k = 0; k <= 4; k += .004)
				{
					taskOneFile << k << ",";
					e = 1;
					datamsg e1 (p, k, e);
					newChannel->cwrite (&e1, sizeof (datamsg)); //ask ecg1
					double reply1;
					newChannel->cread (&reply1, sizeof(double)); //reply
					taskOneFile << reply1 << ",";
					
					e = 2;
					datamsg e2 (p, k, e);
					newChannel->cwrite (&e2, sizeof (datamsg)); //ask ecg2
					double reply2;
					newChannel->cread (&reply2, sizeof(double)); //reply
					taskOneFile << reply2 << "\n";

				}

				fileTransferTracker++;

				if (fileTransferTracker == numChannels) {
					auto stop = std::chrono::high_resolution_clock::now();

					auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
					cout << "time for 1k data pts: " << duration.count() << endl;
				}

				taskOneFile.close();
			}
			//normal data pt request
			else {

				datamsg x (p, t, e);
				newChannel->cwrite (&x, sizeof (datamsg)); // question
				double reply;
				int nbytes = newChannel->cread (&reply, sizeof(double)); //answer
				cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;
			}


			// MESSAGE_TYPE quitMessage = QUIT_MSG;
			// newChannel->cwrite (&quitMessage, sizeof (MESSAGE_TYPE));

			delete newChannel;

			cout << "deleted " << channelNameBuf << endl;


		}

	}



	// task 2 requesting a file
	// condition is filename != ""

	if (filename != ""  && numChannels == 0)
	{

		//open file

		string outpath = "received/" + filename;

		FILE* outfile = fopen (outpath.c_str(), "wb");

		sleep(1);

		filemsg request(0,0);

		int len = sizeof (filemsg) + filename.size()+1;
		char buf4 [len];
		memset(buf4, 0, sizeof(buf4));
		memcpy (buf4, &request, sizeof (filemsg));
		strcpy (buf4 + sizeof (filemsg), filename.c_str());

		chan->cwrite (buf4, len);  // I want the file length;
		__int64_t fileSize = 0;
		chan->cread(&fileSize, sizeof(__int64_t));

		char buf5 [buffercapacity];
		memset(buf5, 0, sizeof(buf5));

		request.offset = 0;
		request.length = buffercapacity;

		
		for (__int64_t i = 0; i < fileSize - buffercapacity; i+= buffercapacity) {

			memset(buf4, 0, sizeof(buf4));
			memcpy (buf4, &request, sizeof (filemsg));
			strcpy (buf4 + sizeof (filemsg), filename.c_str());
			chan->cwrite(&buf4, len);

			memset(buf5, 0, sizeof(buf5));
			chan->cread(&buf5, sizeof(buf5));
			//write to file
			fwrite(buf5, 1, buffercapacity, outfile);

			request.offset += buffercapacity;

		}



		//write remainder
		request.length = fileSize % buffercapacity;

		//if 0, means was perfect multiple of buffercapacity, and buffercapacity bytes are still left
		if (request.length == 0) {
			request.length = buffercapacity;
		}

		memset(buf4, 0, sizeof(buf4));
		memcpy (buf4, &request, sizeof (filemsg));
		strcpy (buf4 + sizeof (filemsg), filename.c_str());
		chan->cwrite(&buf4, len);

		memset(buf5, 0, sizeof(buf5));
		chan->cread(&buf5, sizeof(buf5));

		fwrite(buf5, 1, request.length, outfile);


		//timer end

		auto stop = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
		cout << "File transfer time: " << duration.count() << endl;

		fclose(outfile);

	}

	// task 1 collect 1000 datapts in a file
	// condition is negative time

	else if (t < 0 && numChannels == 0)
	{
		//open file
		std::ofstream taskOneFile;
		taskOneFile.open("received/x1.csv");
		
		//set t to 0, and increment for 1000 data points, p stays same
		char buf [MAX_MESSAGE]; // 256
		for (double k = 0; k <= 4; k += .004)
		{
			taskOneFile << k << ",";
			e = 1;
			datamsg e1 (p, k, e);
			chan->cwrite (&e1, sizeof (datamsg)); //ask ecg1
			double reply1;
			chan->cread (&reply1, sizeof(double)); //reply
			taskOneFile << reply1 << ",";
			
			e = 2;
			datamsg e2 (p, k, e);
			chan->cwrite (&e2, sizeof (datamsg)); //ask ecg2
			double reply2;
			chan->cread (&reply2, sizeof(double)); //reply
			taskOneFile << reply2 << "\n";

		}
		taskOneFile.close();
	}

	if (numChannels == 0) {
    // sending a non-sense message, you need to change this
    char buf [MAX_MESSAGE]; // 256
	memset(buf, 0, sizeof(buf));
    datamsg x (p, t, e);
	chan->cwrite (&x, sizeof (datamsg)); // question
	double reply;
	int nbytes = chan->cread (&reply, sizeof(double)); //answer
	cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;

	}
	
	
	cout << "closing control channel" << endl;
	// closing the channel    
    // MESSAGE_TYPE m = QUIT_MSG;
    // chan->cwrite (&m, sizeof (MESSAGE_TYPE));
	delete chan;
	cout << "deleted" << endl;
	}


	int status;
	pid_t wait_result;

	while ( (wait_result  = wait(&status)) != -1) {
		printf("Process %lu returned result: %d\n", (unsigned long) wait_result, status);
	}

	printf("All children have finished.\n");

	cout << "reached ENDEND" << endl;
	return 0;

}
