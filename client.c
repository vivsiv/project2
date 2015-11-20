/* 
	A client that implements Reliable Data Transfer on top of UDP
	by Vivek Sivakumar and Colin Terndup;
*/
// #include <stdio.h>
// #include <sys/types.h> // definitions of a number of data types used in socket.h and netinet/in.h
// #include <sys/socket.h> // definitions of structures needed for sockets, e.g. sockaddr
// #include <netinet/in.h> // constants and structures needed for internet domain addresses, e.g. sockaddr_in
// #include <netdb.h>      // define structures like hostent
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

#define CLIENT_HOST "localhost"
#define CLIENT_PORT 8100

void error(char *msg){
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]){
	int sockfd;
	int client_port, server_port;
	int filed;
	char *filename, *server_host;
	struct hostent *server;
	struct sockaddr_in serv_addr;

	if (argc < 3){
		error("Error usage <server_host> <server_port> <filename>");
	}

	client_port = CLIENT_PORT;
	server_host = argv[1];
	server_port = atoi(argv[2]);
	filename = argv[3];

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0){
		error("Error opening Socket");
	}

	server = gethostbyname(server_host);
	// client = gethostbyname(CLIENT_HOST);

	if (server == NULL){
		error("Error, no such host");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(server_port);

	Packet request;
	buildHeader(&request, client_port, server_port, FILE_REQUEST, 0, 0, NOT_CORRUPTED, KEEP_ALIVE);
	char* data = filename;
	addData(&request,data);

    if (sendto(sockfd, (char *)&request, sizeof(Packet), 0, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
         error("ERROR sending to socket");	
    }
    int transNum = 0;
    printf("%d)Sent packet: ", transNum);
	printPacket(&request);

    char buffer[PACKET_SIZE];
	bzero(buffer,PACKET_SIZE);
	socklen_t serv_len = sizeof(serv_addr);
	if (recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr *)&serv_addr, &serv_len) < 0){
		error("ERROR reading from socket");
	}

	Packet *response = (Packet *)buffer;
	printf("%d)Received packet: ",transNum);
	printPacket(response);

	//FILE NOT FOUND
	int windowStart = (response->header).seqNumber + 1;
	int windowEnd = windowStart + WINDOW_SIZE;
	int currSeq = windowStart;
	while((response->header).endTrans != END){
		transNum++;
		while (currSeq < windowEnd){
			bzero(&request, sizeof(Packet));
			buildHeader(&request, client_port, server_port, DATA_REQUEST, currSeq ,0, NOT_CORRUPTED, KEEP_ALIVE);
		    if (sendto(sockfd, (char *)&request, sizeof(Packet), 0, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
		         error("ERROR sending to socket");	
		    }
		    printf("%d)Sent packet: ", currSeq);
			printPacket(&request);
			currSeq++;
		}

		bzero(buffer,PACKET_SIZE);
		serv_len = sizeof(serv_addr);
		if (recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr *)&serv_addr, &serv_len) < 0){
			error("ERROR reading from socket");
		}
		Packet *response = (Packet *)buffer;
		printf("%d)Received packet: ", (response->header).seqNumber);
		printPacket(response);
		if ((response->header).seqNumber == windowStart){
			windowStart++;
			windowEnd++;
			// if (windowStart == 2 * WINDOW_SIZE) {
			// 	windowStart = 0;
			// 	windowEnd = WINDOW_SIZE;
			// }
		}
	}

	close(sockfd);

	return 0;
}