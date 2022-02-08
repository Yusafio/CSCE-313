#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
using namespace std;

// Yep, so the general steps:
 
// you pass in the hostname into getaddrinfo (get server's information)
// create a socket based on the server's information
// connect to server
// request from server
// process reply from server

int main(int argc, char** argv) {

    string url = argv[1];


    string filename = "";
    if (argc == 3) {
        filename = argv[2];
    }

    string port_no = "80";

    //parse url

    url:


    int i = 0;
    int urlLength = url.length();
    string curr = url.substr(i, 1);

    //get scheme
    string scheme = "";
    while (i < urlLength) {
        curr = url.substr(i, 1);
        if (url.substr(i, 3) != "://")  {
            scheme = scheme + curr;
            i++;
        }
        else {
            i = i + 3;
            break;
        }
    }

    //get hostname
    bool portChange = false;
    string hostname = "";
    while (i < urlLength) {
        curr = url.substr(i, 1);
        if (curr == "/") {
            i++;
            break;
        }
        else if (curr == ":") {
            portChange = true;
            i++;
            break;
        }
        else {
            hostname = hostname + curr;
            i++;
        }
    }

    // if : hit get port_no
    if (portChange == true) {
        port_no = "";
        while (i < urlLength) {
            curr = url.substr(i, 1);

            if (curr != "/") {
                port_no = port_no + curr;
                i++;
            }
            else {
                i++;
                break;
            }
        }
    }

    //get path

    string path = "/";

    while (i < urlLength) {
        curr = url.substr(i, 1);

        if (curr != "?" || curr != "#") {
            path = path + curr;
            i++;
        }
        else {
            i++;
            break;
        }
    }

    //make own filename if none given
    if (filename == "") {

        if (path == "/") {
            filename = hostname;
        }
        else {
            string token = "";
            curr = "";
            vector<string> stringVector;
            int i = 1;

            while (i < path.length()) {

                curr = path.substr(i, 1);
                if (curr == "/") {
                    stringVector.push_back(token);
                    token = "";
                }
                else {
                    token = token + curr;
                }
                i++;
            }

            if (token != "") {
                stringVector.push_back(token);
            }

            filename = stringVector.back();
        }

    }



    // if (path == "") {
    //     path = "/" ;
    // }

    // cout << "scheme: " << scheme << endl;
    // cout << "hostname: " << hostname << endl;
    // cout << "path: " << path << endl;
    // cout << "port_no " << port_no << endl;

    //tcp connection

    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int status;

    if ((status = getaddrinfo (hostname.c_str(), port_no.c_str(), &hints, &res)) != 0) {
        cerr << "getaddrinfo: " << gai_strerror(status) << endl;
        exit (-1);
    }

    // socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0){
        perror ("client: socket");	
        exit (-1);
    }

    // connect
    if (connect(sockfd, res->ai_addr, res->ai_addrlen)<0){
        perror ("client: connect");
        exit (-1);
    }

    // cout << "connected" << endl;

    //make request


    string request = "GET " + path + " HTTP/1.1\r\nHost: " + hostname + "\r\nConnection:close\r\n\r\n";
    cout << endl;
    cout << "sending the following GET request" << endl;
    cout << endl;
    cout << request;


    // send(sockfd, request.c_str(), strlen(request.c_str()), 0);
    // example,

    // //send HTTP request

    // send(sockfd, req_str.c_str(), req_str.size(), 0);

 

    // //receive response

    // int received_bytes = 0;

    // while((received_bytes = recv(sockfd, buf, 256, 0)) != 0) {

    //     //append to a string or stringstream

    // }

    send(sockfd, request.c_str(), request.size(), 0);

    // string website_HTML = "";
    stringstream website_HTML_SS (stringstream::in | stringstream::out | stringstream::binary);
    int nDataLength;
    char buffer[256];
    memset(&buffer, 0, sizeof buffer);
    while ((nDataLength = recv(sockfd,buffer,256,0)) != 0){
        // cout << "bytes: " << nDataLength << endl;

        website_HTML_SS.write(buffer, nDataLength);  
        // int i = 0;
        // while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r'){

        //     website_HTML+=buffer[i];
        //     i += 1;
        // } 
        // website_HTML = website_HTML + '\0';
              
    }
    // website_HTML = website_HTML + '\0';
    string website_HTML = website_HTML_SS.str();
    close(sockfd);

    // cout << website_HTML;
    string resCode = website_HTML.substr(9, 3);
    string errorCode = website_HTML.substr(9, 1);
    cout << "response code: " << resCode << endl;

    //write to file
    if (resCode == "200") {
        i = 0;
        int start = 0;
        int end = 0;
        curr = "";
        string header = "";
        while (i < website_HTML.length()) {
            curr = website_HTML.substr(i, 1);
            header = header + curr;
            if (website_HTML.substr(i, 4) == "\r\n\r\n") {
                start = i + 4;
                break;
            }

            i++;
        }
        //chunked transfer
        if (header.find("transfer-encoding: chunked") != string::npos) {
            // cout << "entered chunk transfer" << endl;
            i = i + 4;
            bool reading = false;
            string chunkedFile = "";
            while (i < website_HTML.length()) {
                curr = website_HTML.substr(i, 1);
                if(website_HTML.substr(i, 4) == "\r\n\r\n") {
                    //stop reading
                    break;
                }
                else if (website_HTML.substr(i, 2) == "\r\n") {
                    //switch reading
                    // cout << "switched" << endl;
                    reading = !reading;
                    i = i + 1;
                }
                else if (reading){
                    chunkedFile = chunkedFile + curr;
                }
                i++;
            }
            ofstream chunkedOutput(filename, ios::binary);
            chunkedOutput << chunkedFile;
            chunkedOutput.close();

            cout << "created file: " << filename << endl;

        }
        //normal transfer
        else {
            // cout << "entered normal transfer" << endl;
            while (i < website_HTML.length()) {
                curr = website_HTML.substr(i, 7);
                if (curr == "</html>") {
                    end = i + 8;
                    break;
                }
                i++;
            }

            int htmlLength = end - start;
            string htmlFile = website_HTML.substr(start, htmlLength);
            ofstream output(filename, ios::binary);
            output << htmlFile;
            output.close();

            cout << "created file: " << filename << endl;

        }

        ofstream output2("debugHelper", ios::binary);
        output2 << website_HTML;
        output2.close();
    }
    else if (resCode == "301" || resCode == "302") {

        i = 0;
        // cout << website_HTML;
        while (i < website_HTML.length()) {
            if (website_HTML.substr(i, 9) == "location:") {
                i = i + 10;
                string token = "";
                curr = "";
                while (i < website_HTML.length()) {
                    curr = website_HTML.substr(i, 1);
                    if (curr != "\r") {
                        token = token + curr;
                    }
                    else {
                        url = token;
                        goto url;

                    }
                    i++;
                }
            }
            i++;
        }
    }
    else if (errorCode == "4" || errorCode == "5") {
        string errormsg = "error code: " + resCode;
        perror (errormsg.c_str());	
        exit (1);
    }


    cout << "reached end" << endl;
}