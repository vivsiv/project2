/* 
	A server that implements Reliable Data Transfer on top of UDP
	by Vivek Sivakumar and Colin Terndup;
*/
#include <stdio.h>
#include <sys/types.h> // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h> // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h> // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h> // for the waitpid() system call
#include <signal.h>	 //signal name macros, and the kill() prototype */
#include <unistd.h>	
#include <time.h> // get current time for server response
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void error(char *msg){
	perror(msg);
	exit(1);
}

typedef struct {
	//char* sourceHost;
	int sourcePort;
	//char* destHost;
	int destPort;
	int seqNumber;
	int ackField;
	int corrField;
} Header;

typedef struct {
	Header header;
	char data[1024];
} Packet;

void processConnection(int sock){
	int n;
	char buffer[2048];
	char* fileRequested;

	bzero(buffer, 2048);

	//Read from client
	if(read(sock, buffer, 2047) < 0){
		error("ERROR reading from socket");
	}

	printf("%s\n\n", buffer);
}

int main(int argc, char *argv[]){
	int sockfd, newsockfd, process_id;
	int client_port, server_port;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	struct sigaction sa;

	if (argc < 1){
		error("Error usage <server_port>");
	}

	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0){
		error("ERROR opening socket");
	}
	
	server_port = atoi(argv[1]);
	//client_port = atoi(argv[2]);
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(server_port);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		error("ERROR binding socket");
	}

	//listen(sockfd,1);

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
	    perror("sigaction");
	    exit(1);
	}

	while(1){
		//newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

		char buffer[2048];
		bzero(buffer,2048);

		if(recvfrom(sockfd, buffer, 2048, 0, NULL, NULL) < 0){
			error("ERROR reading from socket");
		}

		// printf("%s\n\n", buffer);

		Packet *p = (Packet *)buffer;
		printf("after packet* cast\n");
		printf("%d\n", (p->header).sourcePort);
		printf("%d\n", (p->header).destPort);
	}

	return 0;
}