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

//#define RAND_MAX 100

void error(char *msg){
	perror(msg);
	exit(1);
}

int corruptedPacket(float corrPct){
	float r = (float)(rand() % 100);
	float corrLimit = corrPct * 100.0;
    return r < corrLimit ? CORRUPTED : NOT_CORRUPTED;
}

int lostPacket(float lossPct){
	float r = (float)(rand() % 100);
	float lossLimit = lossPct * 100.0;
    return r < lossLimit ? LOST : NOT_LOST;
}

int main(int argc, char *argv[]){
	int sockfd;
	int client_port, server_port;
	int filed;
	char *filename, *server_host;
	struct hostent *server;
	struct sockaddr_in serv_addr, cli_addr;
	float lossPct, corrPct;

	if (argc < 3){
		error("Error usage <server_host> <server_port> <filename>");
	}

	client_port = CLIENT_PORT;
	server_host = argv[1];
	server_port = atoi(argv[2]);
	filename = argv[3];
	if (argv[4]) lossPct = atof(argv[4]);
	if (argv[5]) corrPct = atof(argv[5]);

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

	Packet fileRequest;
	buildHeader(&fileRequest, client_port, server_port, FILE, 0, TRANS, NOT_CORRUPTED, KEEP_ALIVE);
	char* data = filename;
	addData(&fileRequest,data);

	//FILE REQUEST
    if (sendto(sockfd, (char *)&fileRequest, sizeof(Packet), 0, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
         error("ERROR sending to socket");	
    }
    int transNum = 0;
    printf("Sent File Request: ");
	printPacket(&fileRequest);

    char recv_buffer[PACKET_SIZE];
	bzero(recv_buffer,PACKET_SIZE);
	socklen_t servlen = sizeof(serv_addr);
	if (recvfrom(sockfd, recv_buffer, PACKET_SIZE, 0, (struct sockaddr *)&serv_addr, &servlen) < 0){
		error("ERROR reading from socket");
	}

	close(sockfd);

	Packet *fileResponse = (Packet *)recv_buffer;
	printf("Received File Response: ");
	printPacket(fileResponse);

	if ((fileResponse->header).ackField == FILE_NOT_FOUND) {
		error("File not Found on Server");
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0){
		error("Error opening Socket");
	}

	bzero((char *) &cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	cli_addr.sin_port = htons(client_port);

	bzero((char *) &serv_addr, sizeof(serv_addr));
	servlen = sizeof(serv_addr);

	if (bind(sockfd, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0){
		error("ERROR binding socket");
	}

	int windowStart = 0;
	int windowEnd = windowStart + WINDOW_SIZE;
	int expectedSeq = windowStart;

	Packet *dataRecieved;
	Packet ackSent;
	while(1){
		//printf("Expecting sequence: %d\n", expectedSeq);
		bzero(recv_buffer,PACKET_SIZE);
		if(recvfrom(sockfd, recv_buffer, PACKET_SIZE, 0, (struct sockaddr *)&serv_addr, &servlen) < 0){
			error("ERROR reading from socket");
		}


		dataRecieved = (Packet *)recv_buffer;
		int recv_seq = (dataRecieved->header).seqNumber;
		//printf("Received sequence: %d\n", recv_seq);
		if (recv_seq == expectedSeq){
			printf("%d)Received DATA: ", recv_seq);
			printPacket(dataRecieved);
			int corrupted = corruptedPacket(corrPct);
			//printf("Seq %d Corrupted %d\n", recv_seq, corrupted);
			//if (!corrupted){
				bzero(&ackSent, sizeof(Packet));
				buildHeader(&ackSent, client_port, server_port, DATA, recv_seq, ACK, corrupted, (dataRecieved->header).transAlive);

				if (sendto(sockfd, (char *)&ackSent, sizeof(Packet), 0, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0) {
			        error("ERROR sending to socket");	
			    }
				    
				printf("%d)Sent ACK: ", recv_seq);
				printPacket(&ackSent);
				if ((dataRecieved->header).transAlive == END) break;
				if (!corrupted) expectedSeq++;
				transNum++;
			//}
		}
	}

	close(sockfd);

	return 0;
}