#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "TCPRequestChannel.h"
#include <thread>

using namespace std;

struct DataResponse {
    int pnum;
    double ecgVal;
};

void patient_thread_function(int pnum, int numReq, BoundedBuffer* requestBuf){
    /* What will the patient threads do? */

    datamsg d (pnum, 0.0, 1);

    for (int i = 0; i < numReq; i++) {
        vector<char> datareq ((char*)&d, ((char*)&d) + sizeof(d));
        requestBuf->push(datareq);
        d.seconds += 0.004;
    }
}

void file_thread_function (string filename, BoundedBuffer* requestBuf, TCPRequestChannel* chan, int bcap) {


    string outpath = "received/" + filename;
    //  string outpath = "BIMDC/" + filename;


    FILE* outfile = fopen (outpath.c_str(), "w");
    fclose(outfile);

    int fileSize = sizeof(filemsg) + filename.size() + 1;
    char fileBuf [fileSize];

    filemsg* fm = new (fileBuf) filemsg (0, 0);
    strcpy(fileBuf + sizeof (filemsg), filename.c_str());
    chan->cwrite(fileBuf, sizeof(fileBuf));

    __int64_t fileLength;
    chan->cread(&fileLength, sizeof(fileLength));

    while (fileLength) {
        fm->length = min ( (__int64_t) bcap, fileLength);
        vector<char> fileReq (fileBuf, fileBuf + sizeof(fileBuf));
		requestBuf->push(fileReq);
        fileLength -= fm->length;
        fm->offset += fm->length;
    }

}

void worker_thread_function(BoundedBuffer* requestBuf, TCPRequestChannel* chan, BoundedBuffer* responseBuf, int bcap){
    /*
		Functionality of the worker threads	
    */

   char receivedBuf [bcap];
   while (true) {
       vector<char> reqVec = requestBuf->pop();
       char* request = reqVec.data();
       MESSAGE_TYPE* mtype = (MESSAGE_TYPE*) request;

       if (*mtype == DATA_MSG) {
           chan->cwrite(request, reqVec.size());

           int p = ((datamsg*) request)->person;
           double ecg = 0;

           chan->cread(&ecg, sizeof(double));
           //    cout << "pno: " << p << "ecg: " << ecg << endl;
           DataResponse dr {p, ecg};
           vector<char> drVec ((char*)&dr, ((char*)&dr) + sizeof(dr));
           responseBuf->push (drVec);
       }
       else if (*mtype == FILE_MSG){
           chan->cwrite(request, reqVec.size());
           chan->cread(receivedBuf, sizeof (receivedBuf));
           filemsg* fm = (filemsg*) request;
           string fname = (char *) (fm + 1);
           fname = "received/" + fname;
           FILE* fp = fopen (fname.c_str(), "r+");
           fseek (fp, fm->offset, SEEK_SET);
           fwrite (receivedBuf, 1, fm->length, fp);
           fclose(fp);
       }
       else if (*mtype == QUIT_MSG) {
           chan->cwrite(request, reqVec.size());
           break;
       }
   }
}

void histogram_thread_function (BoundedBuffer* responseBuf, HistogramCollection* hc, int bcap) {
    /*
		Functionality of the histogram threads	
    */

   while (true) {
       vector<char> response = responseBuf->pop();

       MESSAGE_TYPE* mtype = (MESSAGE_TYPE*) response.data();

       DataResponse* dr = (DataResponse*) response.data();
       
    //    //change if nothing else works
    //    if (*mtype == QUIT_MSG) {
    //        break;
    //    }
        if (dr->pnum == -1) {
			break;
		}
       
       hc->update(dr->pnum, dr->ecgVal);

   }
}



int main(int argc, char *argv[])
{
    // cout << "starting" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    vector<char> qVec ((char*)&q, (char*)&q + sizeof(q));

    int n = 100;    		//default number of requests per "patient"
    int p = 10;     		// number of patients [1,15]
    int w = 100;    		//default number of worker threads
    int b = 20; 		// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer

    int h = 50;             //default number of histogram threads

    string hostname = "";
    string portno = "";

    srand(time_t(NULL));
    
    //get arguments from user
    
    // cout << "initializing fork" << endl;
    // int pid = fork();

    // if (pid < 0) {
	// 	perror("Fork failed!");
	// 	return 2;
	// }

    // if (pid == 0){
	// 	// modify this to pass along m
    //     // execl ("server", "server", (char *)NULL);

    //     argv[0] = "./server";
	// 	execvp(argv[0], argv);
    //     return 2;
    // }

    // cout << "getting opts" << endl;

    // cout << "about to read args" << endl;
    int opt  = -1;
    string filename = "";

    while ((opt = getopt(argc, argv, "n:p:h:w:b:m:f:a:r:")) != -1) {
		switch (opt) {
            case 'n':
                n = atoi (optarg);
                break;
            case 'p':
                p = atoi (optarg);
                break;
            case 'h':
                h = atoi (optarg);
                break;
			case 'w':
				w = atoi (optarg);
				break;
			case 'b':
				b = atoi (optarg);
				break;
			case 'm':
				m = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
            case 'a':
                hostname = optarg;
                break;
            case 'r':
                portno = optarg;
                break;

		}
	}

    // cout << "read arguments" << endl;
    
	// FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    //make tcp her
    //comment out later
    TCPRequestChannel* chan = new TCPRequestChannel(hostname, portno);
    // cout << "made control channel" << endl;
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
	HistogramCollection hc;

    struct timeval start, end;
    gettimeofday (&start, 0);

    // cout << "creating channels" << endl;

    //creating channels
    // char nameBuffer[1024];
    // TCPRequestChannel* channels [w];
    // for (int i = 0; i < w; i++) {
    //     MESSAGE_TYPE ncm = NEWCHANNEL_MSG;
    //     chan->cwrite (&ncm, sizeof(ncm));
    //     chan->cread(nameBuffer, sizeof(nameBuffer));
    //     // channels[i] = new FIFORequestChannel (nameBuffer, FIFORequestChannel::CLIENT_SIDE);
    //     channels[i] = new TCPRequestChannel (nameBuffer, portno);
    // }

    
    // TCPRequestChannel** channels = new TCPRequestChannel*[w];

    // cout << "made array" << endl;
    // for (int i = 0; i < w; i++) {
    //     channels[i] = new TCPRequestChannel(hostname, portno);
    // }

    TCPRequestChannel* channels [w];

    // cout << "made array" << endl;
    for (int i = 0; i < w; i++) {
        channels[i] = new TCPRequestChannel(hostname, portno);
    }

    // cout << "made channels" << endl;

    // cout << "creating worker threads" << endl;

    thread workers [w];
    //creating worker threads
    for (int i = 0; i < w; i++) {
        workers[i] = thread(worker_thread_function, &request_buffer, channels[i], &response_buffer, m);
    }

    thread patients [p];
    thread histogramThreads[h];
    thread filethread;

    if(filename == "") {

        // cout << "filling histogram collection" << endl;
        //fill histogram collection
        for (int i = 0; i < p; i++) {
            Histogram* h = new Histogram (10, -4.0, 4.0);
            hc.add(h);
        }
        
        // cout << "creating patient threads" << endl;
        //creating patient threads
        for (int i = 0; i < p; i++) {
            patients[i] = thread(patient_thread_function, i + 1, n, &request_buffer);
        }

        // cout << "creating histogram threads" << endl;
        //creating histogram threads
        for (int i = 0; i < h; i++) {
            histogramThreads[i] = thread(histogram_thread_function, &response_buffer, &hc, m);
        }

        //join threads
        // cout << "joining threads" << endl;
        for (int i = 0; i < p; i++) {
            patients[i].join();
        }

    }
    else {
        filethread = thread (file_thread_function, filename, &request_buffer, chan, m);
        filethread.join();
    }

    // cout << "reached checkpoint" << endl;

    //join threads
    for (int i = 0; i < w; i++) {
        request_buffer.push(qVec);
    }

    for (int i = 0; i < w; i++) {
        workers[i].join();
    }

    if (filename == "") {

        DataResponse rq {-1, 0.0};
        vector<char> rqVec ((char*)&rq, (char*)&rq + sizeof(rq));
        for (int i = 0; i < h; i++) {
            response_buffer.push(rqVec);
        }

        for (int i = 0; i < h; i++) {
            histogramThreads[i].join();
        }

    }

    /* Start all threads here */
	

	/* Join all threads here */
    gettimeofday (&end, 0);
    // print the results
    if (filename == "") {
        hc.print ();
    }
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));

    for (int i = 0; i < w; i++) {
        delete channels[i];
    }

    cout << "All Done!!!" << endl;
    delete chan;
    
}
