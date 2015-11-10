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

void error(char *msg){
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]){
	int sockfd, newsockfd, process_id;
	int client_port, server_port;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	if (argc < 3){
		error("Error usage <server_port> <client_port>");
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0){
		error("ERROR opening socket");
	}

	server_port = atoi(argv[1]);
	client_port = atoi(argv[2]);

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(server_port);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr) < 0)){
		error("ERROR binding socket");
	}

	listen(sockfd,1);

	while(1){
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

		//Fork off a new process
		process_id = fork();
		if (process_id < 0){
			error("ERROR on fork");
		}
		//Close the parent process in child processes
		if (process_id == 0){
			close(sockfd);
			
			//do_something();
			
			close(newsockfd);
			exit(0);
		}
		//Close child processes in the parent process
		else {
			close(newsockfd);
		}
	}

	return 0;
}