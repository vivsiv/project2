#include <stdio.h>
#include <sys/types.h> // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h> // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h> // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h> // for the waitpid() system call
#include <signal.h>	 //signal name macros, and the kill() prototype */
#include <unistd.h>	
#include <time.h> // get current time for server response
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define FILE_REQUEST 1
#define DATA_REQUEST 0

typedef struct {
	//char* sourceHost;
	int sourcePort;
	//char* destHost;
	int destPort;
	int reqField;
	int seqNumber;
	int ackField;
	int corrField;
} Header;

typedef struct {
	Header header;
	char data[1024];
} Packet;

void buildHeader(Packet *p, int srcPort, int destPort, int reqField, int seqNumber, int ackField, int corrField){
	(p->header).sourcePort = srcPort;
	(p->header).destPort = destPort;
	(p->header).reqField = reqField;
	(p->header).seqNumber = seqNumber;
	(p->header).ackField = ackField;
	(p->header).corrField = corrField;
}

void addData(Packet *p, char *data){
	bcopy(data, p->data, strlen(data));
}