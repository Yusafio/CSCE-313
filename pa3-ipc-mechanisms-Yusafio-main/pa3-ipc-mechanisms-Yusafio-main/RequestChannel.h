#ifndef _RequestChannel_H_
#define _RequestChannel_H_


#include <string>
class RequestChannel {

public:
    typedef enum {SERVER_SIDE, CLIENT_SIDE} Side;
    typedef enum {READ_MODE, WRITE_MODE} Mode;
    string my_name;

private:
    // string my_name;
	Side my_side;
public:

    /* CONSTRUCTOR/DESTRUCTOR */
    RequestChannel (const std::string _name, const Side _side) {
        my_name = _name;
        my_side = _side;
    }
    virtual ~RequestChannel() {}
    /* destruct operation should be derived class-specific */

    virtual int cread (void* msgbuf, int bufcapacity) = 0;
    /* Blocking read; returns the number of bytes read.
    If the read fails, it returns -1. */

    virtual int cwrite (void* msgbuf, int bufcapacity) = 0;
    /* Write the data to the channel. The function returns
    the number of characters written, or -1 when it fails */
};

#endif