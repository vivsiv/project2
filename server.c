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
	
	int transNum = 0;
	int filed;

	clilen = sizeof(cli_addr);

	char recv_buffer[PACKET_SIZE];
	bzero(recv_buffer,PACKET_SIZE);

	Packet *fileRequest;
	Packet fileResponse;

	//FILE REQUEST LOOP
	while(1){
		if(recvfrom(sockfd, recv_buffer, PACKET_SIZE, 0, (struct sockaddr *)&cli_addr, &clilen) < 0){
			error("ERROR reading from socket");
		}

		fileRequest = (Packet *)recv_buffer;
		printf("Received File Request: ");
		printPacket(fileRequest);

		if ((fileRequest->header).reqField == 1){
			filed = open(fileRequest->data, O_RDONLY);
			if (filed < 0) buildHeader(&fileResponse, (fileRequest->header).destPort, (fileRequest->header).sourcePort, FILE, 0, FILE_NOT_FOUND, NOT_CORRUPTED, END);
			else buildHeader(&fileResponse, (fileRequest->header).destPort, (fileRequest->header).sourcePort, FILE, 0, TRANS, NOT_CORRUPTED, KEEP_ALIVE);

			if (sendto(sockfd, (char *)&fileResponse, sizeof(Packet), 0, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr_in)) < 0) {
		        error("ERROR sending to socket");	
		    }
		    client_port = fileResponse.header.destPort;
		 break;
		}
	}

	close(sockfd);
	if (filed < 0) return 0;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0){
		error("Error opening Socket");
	}

	client = gethostbyname(CLIENT_HOST);
	bzero((char *) &cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	bcopy((char *)client->h_addr, (char *)&cli_addr.sin_addr.s_addr, client->h_length);
	cli_addr.sin_port = htons(client_port);
	clilen = sizeof(cli_addr);
	//DATA REQUEST LOOP
	int windowStart = 0;
	int windowEnd = windowStart + WINDOW_SIZE;
	int currSeq = windowStart;

	int b_read;
	char readBuf[MAX_DATA];
	bzero(readBuf,MAX_DATA);

	Packet dataSent;
	Packet *ackRecieved;

	int endAck = -1;
	int TRANS_ALIVE = KEEP_ALIVE;
	while(1){
		transNum++;
		while (TRANS_ALIVE && currSeq < windowEnd){
			//Read File
			bzero(readBuf, MAX_DATA);
			b_read = read(filed, readBuf, MAX_DATA - 1);
			TRANS_ALIVE = b_read > 0 ? KEEP_ALIVE : END;
			//Prepare Dat packet
			bzero(&dataSent, sizeof(Packet));
			buildHeader(&dataSent, server_port, client_port, DATA, currSeq , TRANS, NOT_CORRUPTED, TRANS_ALIVE);
			addData(&dataSent, readBuf);
			//Send Data packet
		    if (sendto(sockfd, (char *)&dataSent, sizeof(Packet), 0, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr)) < 0) {
		         error("ERROR sending to socket");	
		    }
		    printf("%d)Sent DATA: ", currSeq);
			printPacket(&dataSent);
			if (b_read == 0) endAck = currSeq;
			currSeq++;
		}
		//Prepare receiving packet;
		bzero(recv_buffer,PACKET_SIZE);
		if (recvfrom(sockfd, recv_buffer, PACKET_SIZE, 0, (struct sockaddr *)&cli_addr, &clilen) < 0){
			error("ERROR reading from socket");
		}
		ackRecieved = (Packet *)recv_buffer;
		printf("%d)Received ACK: ", (ackRecieved->header).seqNumber);
		printPacket(ackRecieved);
		if ((ackRecieved->header).seqNumber == endAck) break;
		else if ((ackRecieved->header).seqNumber == windowStart){
			windowStart++;
			windowEnd++;
			// if (windowStart == 2 * WINDOW_SIZE) {
			// 	windowStart = 0;
			// 	windowEnd = WINDOW_SIZE;
			// }
		}
	}

	close(sockfd);


	// while(1){
	// 	bzero(buffer,PACKET_SIZE);
	// 	bzero(fileBuf, MAX_DATA);

	// 	if(recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr *)&cli_addr, &clilen) < 0){
	// 		error("ERROR reading from socket");
	// 	}

	// 	request = (Packet *)buffer;
	// 	if ((request->header).seqNumber >= windowStart && (request->header).seqNumber < windowEnd){
	// 	printf("%d)Received packet: ", transNum);
	// 	printPacket(request);

	// 	bread = read(filed, fileBuf, MAX_DATA - 1);
	// 	int end  = bread > 0 ? KEEP_ALIVE : END;
	// 	bzero(&response, sizeof(Packet));

	// 	buildHeader(&response, (request->header).destPort, (request->header).sourcePort, FILE_REQUEST, (request->header).seqNumber, 0, NOT_CORRUPTED, end);
	// 	addData(&response, fileBuf);

	// 	if (sendto(sockfd, (char *)&response, sizeof(Packet), 0, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr_in)) < 0) {
	//         error("ERROR sending to socket");	
	//     }
		    
	// 	printf("%d)Sent packet: ", transNum);
	// 	printPacket(&response);
	// 	if (bread == 0) break;
	// 	transNum++;
	// }

	return 0;
}