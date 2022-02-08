#include "common.h"
#include "TCPRequestChannel.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
using namespace std;


TCPRequestChannel::TCPRequestChannel (const string hostname, const string port_no) {

    if (hostname == "") {
        //server

        struct addrinfo hints, *serv;
        struct sockaddr_storage their_addr;
        socklen_t sin_size;
        char s[INET6_ADDRSTRLEN];
        int rv;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if ((rv = getaddrinfo(NULL, port_no.c_str(), &hints, &serv)) != 0) {
            cerr  << "getaddrinfo: " << gai_strerror(rv) << endl;
            exit (-1);
        }
        if ((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) {
            perror("server: socket");
            exit (-1);
        }
        if (bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            exit (-1);
        }
        freeaddrinfo(serv);

        if (listen(sockfd, 20) == -1) {
            perror("listen");
            exit(1);
        }

        cout << "server ready, waiting" << endl;
        
    }
    else {
        //client

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
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0){
            perror ("client: socket");	
            exit (-1);
        }

        // connect
        if (connect(sockfd, res->ai_addr, res->ai_addrlen)<0){
            perror ("client: connect");
            exit (-1);
        }

        // cout << "Connected" << endl;


    }
}

//used by server
TCPRequestChannel::TCPRequestChannel (int sockfd) {
    this->sockfd = sockfd;
}

//destructor
TCPRequestChannel::~TCPRequestChannel() {
    close(sockfd);
}

int TCPRequestChannel::cread (void* msgbuf, int buflen) {
    return recv(sockfd, msgbuf, buflen, 0);
}
int TCPRequestChannel::cwrite (void* msgbuf , int msglen) {
    return send(sockfd, msgbuf, msglen, 0);
}


int TCPRequestChannel::getfd () {
    return sockfd;
}