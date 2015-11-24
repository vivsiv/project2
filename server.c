/* 
	A server that implements Reliable Data Transfer on top of UDP
	by Vivek Sivakumar and Colin Terndup;
*/

#include "rdt_packet.h"

#define RTO 50

static int TIMEOUT = 0;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void error(char *msg){
	perror(msg);
	exit(1);
}

void timeout(void) {
  TIMEOUT = 1;
  //printf("Timeout\n");
}

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
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(server_port);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		error("ERROR binding socket");
	}

	//Reap all dead processes
	sa.sa_handler = sigchld_handler; 
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
	    perror("sigaction");
	    exit(1);
	}
	
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
		    printf("Sent File Response: ");
			printPacket(&fileResponse);
		    client_port = fileResponse.header.destPort;
		 break;
		}
	}

	close(sockfd);
	if (filed < 0) {
		error("File not found");
	}
	//END FILE REQUEST

	//START DATA TRANSMISSION
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

	int windowStart = 0;
	int windowEnd = windowStart + WINDOW_SIZE;
	int currSeq = windowStart;
	Packet *window[WINDOW_SIZE];
	int windowIdx = 0;

	int b_read;
	char readBuf[MAX_DATA];
	bzero(readBuf,MAX_DATA);

	int lastSeq = -1;
	int lastAck = -1;
	int TRANS_ALIVE = KEEP_ALIVE;

	Packet *dataSent;
	Packet *ackRecieved;

	struct itimerval window_timer;	/* for setting itimer */
	(void) signal(SIGALRM, (void (*)(int))timeout);
	window_timer.it_value.tv_sec = RTO/1000;
  	window_timer.it_value.tv_usec = (RTO * 1000) % 1000000;
  	window_timer.it_interval = window_timer.it_value;
  	//setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&(window_timer.it_interval),sizeof(struct timeval));

  	//setitimer(ITIMER_REAL, &window_timer, NULL);
	while(1){
		printf("Window %d-%d\n", windowStart, windowEnd);
		//RETRANSMIT
		if (lastAck < currSeq - 1){
			//TIMEOUT = 0;
			//setitimer(ITIMER_REAL, &window_timer, NULL);
			int retransNum = lastAck + 1;
			int retransIdx = 0;
			while (retransNum < currSeq && retransIdx < WINDOW_SIZE){
				//printf("lastAck: %d\n", lastAck);
				//printf("Packet accessed %d\n", (lastAck + 1) % WINDOW_SIZE);
				
				dataSent = window[retransIdx];
				//Check if seq num == retransNum if so send
				if (retransNum == (dataSent->header).seqNumber){
					if (sendto(sockfd, (char *)dataSent, sizeof(Packet), 0, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr)) < 0) {
				         error("ERROR sending to socket");	
				    }
				    printf("%d)RE-Sent DATA: ", (dataSent->header).seqNumber);
					printPacket(dataSent);
					retransNum++;
					
				}
				retransIdx++;
			}
		}
		//REGULAR TRANSMIT
		while (TRANS_ALIVE && currSeq < windowEnd){
			//Read File
			bzero(readBuf, MAX_DATA);
			b_read = read(filed, readBuf, MAX_DATA - 1);
			TRANS_ALIVE = b_read > 0 ? KEEP_ALIVE : END;

			dataSent = (Packet *)malloc(sizeof(Packet));
			buildHeader(dataSent, server_port, client_port, DATA, currSeq , TRANS, NOT_CORRUPTED, TRANS_ALIVE);
			addData(dataSent, readBuf);
			window[windowIdx] = dataSent;
			if (windowIdx < windowEnd) windowIdx++;
			//Send Data packet
		    if (sendto(sockfd, (char *)dataSent, sizeof(Packet), 0, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr)) < 0) {
		         error("ERROR sending to socket");	
		    }
		    printf("%d)Sent DATA: ", currSeq);
			printPacket(dataSent);
			if (b_read == 0) lastSeq = currSeq;
			currSeq++;
			//printf("Current Seq %d\n", currSeq);
		}
		//RECEIVE ACK;
		bzero(recv_buffer,PACKET_SIZE);
		if (recvfrom(sockfd, recv_buffer, PACKET_SIZE, 0, (struct sockaddr *)&cli_addr, &clilen) < 0){
			error("ERROR reading from socket");
		}
		ackRecieved = (Packet *)recv_buffer;
		if (!(ackRecieved->header).corrField) lastAck = (ackRecieved->header).seqNumber;
		printf("%d)Received ACK: ", (ackRecieved->header).seqNumber);
		printPacket(ackRecieved);
		if ((ackRecieved->header).seqNumber == lastSeq) break;
		else if (lastAck == windowStart && lastSeq == -1){
			windowStart++;
			windowEnd++;
			//setitimer(ITIMER_REAL, &window_timer, NULL);
			//windowStart = clock();
			free(window[0]);
			memcpy(window, window + 1, (WINDOW_SIZE - 1) * sizeof(Packet *));
			windowIdx--;
		}
	}

	close(sockfd);
	//END DATA TRANSMISSION
	return 0;
}