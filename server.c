/* 
	A server that implements Reliable Data Transfer on top of UDP
	by Vivek Sivakumar and Colin Terndup;
*/
// #include <stdio.h>
// #include <sys/types.h> // definitions of a number of data types used in socket.h and netinet/in.h
// #include <sys/socket.h> // definitions of structures needed for sockets, e.g. sockaddr
// #include <netinet/in.h> // constants and structures needed for internet domain addresses, e.g. sockaddr_in
// #include <stdlib.h>
// #include <strings.h>
// #include <sys/wait.h> // for the waitpid() system call
// #include <signal.h>	 //signal name macros, and the kill() prototype */
// #include <unistd.h>	
// #include <time.h> // get current time for server response
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <string.h>

#include "rdt_packet.h"

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void error(char *msg){
	perror(msg);
	exit(1);
}

// void processPacket(Packet *p){
// 	char buffer[2048];
// 	bzero(buffer, 2048);
// 	Packet* resp;
// 	if ((p->header).reqField == 1){
		
// 		else {
// 			buildHeader(resp, (p->header).destPort, (p->header).sourcePort, FILE_REQUEST, 1, 0);
// 		}
// 	}

// 	printf("%s\n\n", buffer);
// }

int main(int argc, char *argv[]){
	int sockfd, newsockfd, process_id;
	int client_port, server_port;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	struct sigaction sa;
	struct hostent *client;

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
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
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
		char buffer[2048];
		bzero(buffer,2048);

		socklen_t clilen = sizeof(cli_addr);
		if(recvfrom(sockfd, buffer, 2048, 0, (struct sockaddr *)&cli_addr, &clilen) < 0){
			error("ERROR reading from socket");
		}

		Packet *p = (Packet *)buffer;
		printf("Received packet from address:%d\n",cli_addr.sin_addr.s_addr);
		printf("Received packet from port:%d\n",cli_addr.sin_port);
		Packet response;
		int filed;
		if ((p->header).reqField == 1){
			printf("A file request was sent!\n");
			int filed = open(p->data, O_RDONLY);
			printf("filed: %d\n",filed);
			if (filed < 0){
				printf("Building file NOT found response packet!\n");
				buildHeader(&response, (p->header).destPort, (p->header).sourcePort, FILE_REQUEST, 0, FILE_NOT_FOUND, 0);
				printf("Response: file_request:%d, file_found:%d\n", response.header.reqField, response.header.ackField);
			}
			else{
				printf("Building a file FOUND response packet!\n");
				buildHeader(&response, (p->header).destPort, (p->header).sourcePort, FILE_REQUEST, 0, 1, 0);
				printf("Response: file_request:%d, file_found:%d\n", response.header.reqField, response.header.ackField);
			}
			if (sendto(sockfd, (char *)&response, sizeof(Packet), 0, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr_in)) < 0) {
		        error("ERROR sending to socket");	
		    }
			
		}
		// else {
		// 	while (0){

		// 	}
		// }
		


		// printf("after packet* cast\n");
		// printf("%d\n", (p->header).sourcePort);
		// printf("%d\n", (p->header).destPort);
	}

	return 0;
}