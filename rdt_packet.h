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

#define CLIENT_HOST "localhost"
#define CLIENT_PORT 8100
#define SERVER_HOST "localhost"

#define FILE_REQUEST 1
#define DATA_REQUEST 0
#define FILE_NOT_FOUND -1
#define MAX_DATA 1024

#define PACKET_SIZE 2048

#define CORRUPTED 1
#define NOT_CORRUPTED 0

#define END 1
#define KEEP_ALIVE 0

#define WINDOW_SIZE 3

typedef struct {
	//char* sourceHost;
	int sourcePort;
	//char* destHost;
	int destPort;
	int reqField;
	int seqNumber;
	int ackField;
	int corrField;
	int endTrans;
} Header;

typedef struct {
	Header header;
	char data[MAX_DATA];
} Packet;

void buildHeader(Packet *p, int srcPort, int destPort, int reqField, int seqNumber, int ackField, int corrField, int endTrans){
	(p->header).sourcePort = srcPort;
	(p->header).destPort = destPort;
	(p->header).reqField = reqField;
	(p->header).seqNumber = seqNumber;
	(p->header).ackField = ackField;
	(p->header).corrField = corrField;
	(p->header).endTrans = endTrans;
}

void addData(Packet *p, char *data){
	bcopy(data, p->data, strlen(data));
}

void printPacket(Packet *p){
	char dataSample[11];
	strncpy(dataSample, p->data, 10);
	dataSample[10] = '\0';
	
	
	// strncat(dataSample,"\0",1);
	printf("seq:%d|src:%d|dest:%d|ack:%d|req:%d|end:%d|data:%s\n",(p->header).seqNumber, (p->header).sourcePort, (p->header).destPort,(p->header).ackField, (p->header).reqField, (p->header).endTrans, dataSample);

}