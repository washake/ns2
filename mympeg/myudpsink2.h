#ifndef myudpsink2_h
#define myudpsink2_h

#include <stdio.h>
#include "agent.h"

class myUdpSink2 : public Agent
{
public:
 		myUdpSink2() : Agent(PT_UDP), pkt_received(0) {} 		
        	void recv(Packet*, Handler*);
        	int command(int argc, const char*const* argv);
        	void print_status();
protected:
		char tbuf[100];
		FILE *tFile;
		unsigned long int  pkt_received;
};

#endif
