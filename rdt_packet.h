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
#include <sys/time.h>

#define CLIENT_HOST "localhost"
#define CLIENT_PORT 8100
#define SERVER_HOST "localhost"

#define FILE 1
#define DATA 0
#define MAX_DATA 1024

#define PACKET_SIZE 2048

#define CORRUPTED 1
#define NOT_CORRUPTED 0

#define ACK 1
#define TRANS 0
#define FILE_NOT_FOUND -1

#define END 0
#define KEEP_ALIVE 1

#define LOST 1
#define NOT_LOST 0

#define WINDOW_SIZE 3

typedef struct {
	int sourcePort;
	int destPort;
	int reqField;
	int seqNumber;
	int ackField;
	//int corrField;
	int transAlive;
} Header;

typedef struct {
	Header header;
	char data[MAX_DATA];
} Packet;

void buildHeader(Packet *p, int srcPort, int destPort, int reqField, int seqNumber, int ackField, int transAlive){
	(p->header).sourcePort = srcPort;
	(p->header).destPort = destPort;
	(p->header).reqField = reqField;
	(p->header).seqNumber = seqNumber;
	(p->header).ackField = ackField;
	//(p->header).corrField = corrField;
	(p->header).transAlive = transAlive;
}

void addData(Packet *p, char *data){
	bcopy(data, p->data, strlen(data));
}

void printPacket(Packet *p){
	char dataSample[11];
	strncpy(dataSample, p->data, 10);
	dataSample[10] = '\0';
	printf("seq:%d|src:%d|dest:%d|ack:%d|req:%d|alive:%d|data:%s\n",(p->header).seqNumber, (p->header).sourcePort, (p->header).destPort,(p->header).ackField, (p->header).reqField, (p->header).transAlive, dataSample);
}