/* 
	A client that implements Reliable Data Transfer on top of UDP
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
	int sockfd;
	int client_port, server_port;
	int filed;
	char* filename;
	struct hostnet *server;
	struct sockaddr_in serv_addr;

	if (argc < 4){
		error("Error usage <client_port> <server_port> <filename>");
	}

	client_port = atoi(argv[1]);
	server_port = atoi(argv[2]);
	filename = argv[3];
	filed = open(filename, O_RDONLY);

	if (filed < 0){
		error("Error file not found");
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0){
		error("Error opening Socket");
	}

	//server = gethostbyname()

	if (server == NULL){
		error("Error, no such host");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(server_port);

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		error("Error connecting to server");
	}

	//do_stuff();

	close(sockfd);

	return 0;
}