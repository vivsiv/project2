/* 
	A receiver/client that implements Reliable Data Transfer on top of UDP
	by Vivek Sivakumar and Colin Terndrup;
*/

#include "rdt_packet.h"

#define CLIENT_HOST "localhost"
#define CLIENT_PORT 8100


static int WINDOW_SIZE = 3;

int main(int argc, char *argv[]){
	int sockfd;
	int client_port, server_port;
	char *filename, *server_host;
	struct hostent *server;
	struct sockaddr_in serv_addr, cli_addr;
	float lossPct, corrPct;

	if (argc < 3){
		error("Error usage <sender_host> <sender_port> <filename>");
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

	if (server == NULL){
		error("Error, no such host");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(server_port);

	Packet fileRequest;
	buildHeader(&fileRequest, client_port, server_port, FILE, 0, TRANS, KEEP_ALIVE, FILE_WINDOW);
	char* data = filename;
	addData(&fileRequest,data);

	//FILE REQUEST
    if (sendto(sockfd, (char *)&fileRequest, sizeof(Packet), 0, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
         error("ERROR sending to socket");	
    }

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
	WINDOW_SIZE = (fileResponse->header).windowSize;

	if ((fileResponse->header).ackField == FILE_NOT_FOUND) {
		error("File not Found on Server");
	}
	//END FILE REQUEST

	//START RECEIVING DATA
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

	int filed = open("out.txt", O_RDWR);
	printf("got file out%d\n", filed);
	printf("Receiving Window Size:%d\n", WINDOW_SIZE);
	while(1){
		//RECEIVE DATA
		bzero(recv_buffer,PACKET_SIZE);
		if(recvfrom(sockfd, recv_buffer, PACKET_SIZE, 0, (struct sockaddr *)&serv_addr, &servlen) < 0){
			error("ERROR reading from socket");
		}


		dataRecieved = (Packet *)recv_buffer;
		int recv_seq = (dataRecieved->header).seqNumber;
		int corrupted = corruptedPacket(corrPct);
		int lost = lostPacket(lossPct);
		if (corrupted) printf("Seq %d) Corrupted! ", recv_seq);
		if (lost) printf("Seq %d) Lost! ", recv_seq);
		if (corrupted || lost) printf("\n");
		//SEND ACK
		if (!corrupted && !lost){
			if (recv_seq < expectedSeq){
				printf("%d)Received OLD DATA: ", recv_seq);
				printPacket(dataRecieved);
				bzero(&ackSent, sizeof(Packet));
				buildHeader(&ackSent, client_port, server_port, DATA, expectedSeq - 1, ACK, (dataRecieved->header).transAlive, WINDOW_SIZE);

				if (sendto(sockfd, (char *)&ackSent, sizeof(Packet), 0, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0) {
			        error("ERROR sending to socket");	
			    }
				    
				printf("%d)Sent OLD ACK: ", recv_seq);
				printPacket(&ackSent);
			}
			else if (recv_seq == expectedSeq){
				printf("%d)Received NEW DATA: ", recv_seq);
				printPacket(dataRecieved);

				bzero(&ackSent, sizeof(Packet));
				buildHeader(&ackSent, client_port, server_port, DATA, recv_seq, ACK, (dataRecieved->header).transAlive, WINDOW_SIZE);

				if (sendto(sockfd, (char *)&ackSent, sizeof(Packet), 0, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0) {
			        error("ERROR sending to socket");	
			    }
				    
				printf("%d)Sent NEW ACK: ", recv_seq);
				printPacket(&ackSent);

				write(filed, dataRecieved->data, (dataRecieved->header).dataSize);
				if ((dataRecieved->header).transAlive == END) break;
				expectedSeq++;
			}
		}
	}

	close(filed);
	close(sockfd);

	return 0;
}