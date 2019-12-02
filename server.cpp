#include <arpa/inet.h> 
#include <errno.h> 
#include <netinet/in.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <strings.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <fstream>

#define PORT 			5132
#define BUFFER_SIZE 	1024
#define NUM_BYTES_CHUNK 1024
#define ONE_BYTE		1

char fname[100];
int endFlag;
time_t time_start, time_end;
double sec;

void signal_handler(int signal) 
{
	if (signal == SIGINT)
		endFlag = 1;
} 

int max(int x, int y) {

	if (x > y)
		return x;
	else 
		return y;
}

void sending_file(int connfd)
{
	std::ofstream logfile;

	logfile.open("sendLogFile.csv");

	printf("connection accepted and id: %d\n", connfd);
	logfile << "Connection id: " << connfd << std::endl; 

	// sending file name to client 
	if ((write(connfd, fname, 256)) == (-1))
		printf("write fname(): %s\n", strerror(errno));
	logfile << "Filename: " << fname << std::endl;

	// opening file that will be send
	FILE *fp = fopen(fname, "rb");
	if (fp == NULL) 
		printf("Open file error.\n");

	time(&time_start);

	endFlag = 0;
	while (endFlag == 0) 
	{
		// prepare a buffer in NUM_BYTES_CHUNK bytes 
		unsigned char buffer[NUM_BYTES_CHUNK] = {0};

		// copy data from fname per 1024 bytes 
		// to buffer, second arg is size of each chunk(in bytes)
		// third args is number of chunk(in bytes)
		int nbytesRead = fread(buffer, ONE_BYTE, (NUM_BYTES_CHUNK), fp);

		// if its read more than 0 bytes, then send chunk to client
		if (nbytesRead > 0)
			if ((write(connfd, buffer, nbytesRead)) == (-1))
				printf("write error: %s\n",strerror(errno));
			// else counter_time ++;

	
		if (nbytesRead < NUM_BYTES_CHUNK) 
		{
			if (feof(fp)) {
				printf("Transfer completed for id: %d\n", connfd);
				
				time(&time_end);
				sec = difftime(time_end, time_start);
				logfile << "size of buffer: " << NUM_BYTES_CHUNK << " bytes." << std::endl;
				logfile << "Time elapsed: " << sec << " seconds."<< std::endl;
			}

			if (ferror(fp)) 
				printf("Error reading.\n");
			break;
		}
	} // end while
	
	printf("Time elapsed: %g seconds\n", sec);
	printf("Closing connection for id: %d\n", connfd);
	close(connfd);

}

int main(int argc, char *argv[])
{
	int listenfd, udpfd, maxfdp1, nready, connfd, err;
	struct sockaddr_in serv_addr, client_addr;
	fd_set rset;
	socklen_t len;
	ssize_t n;
	int bytes;
	pthread_t tid;
	pid_t childpid;
	const int on = 1;
	char bufferFname[BUFFER_SIZE];
	char* message = "Hello client";
	void sig_chld(int);
	struct timeval timer;
	


	// create signal handle
	if (signal(SIGINT, signal_handler) == SIG_ERR)
		printf("signal_handler error.\n");

	// create lsitening TCP socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("socket(): %s\n", strerror(errno));


	printf("Socket retrieve successfully\n");
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	// reuse port 
	int reuse = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

	// binding serv_addr structur to listenfd 
	if ((bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0)
		printf("bind(): %s\n", strerror(errno));
	
	if ((listen(listenfd, 10)) < 0)
		printf("listen(): %s\n", strerror(errno));


	// Create UDP socket
	if ((udpfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		printf("socket_udp(): %s\n", strerror(errno));


	// binding serv_addr structur to udpfd
	if ((bind(udpfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0)
		printf("bind_udp(): %s\n", strerror(errno));

	// clear the descriptor set
	FD_ZERO(&rset);

	// get maxfd
	maxfdp1 = (max(listenfd, udpfd)) + 1;

	// select the file that will be send
	if (argc < 2) {
		printf("Enter file name: ");
		//gets(fname);
	} else strcpy(fname, argv[1]);

	// set timer 
	timer.tv_sec = 5;
	timer.tv_usec = 0;

	endFlag = 0;

	while(endFlag == 0) 
	{
		printf("Waiting for connection ...\n");

		// set listenfd and udpfd in readset
		FD_SET(listenfd, &rset);
		FD_SET(udpfd, &rset);

		// select the ready descriptor
		nready = select(maxfdp1, &rset, NULL, NULL, &timer);

		if (nready == -1)
			printf("Error in select.\n");
		else if (nready == 0)
			printf("Timeout. no data after 5 second.\n");

		else {
			// if tcp socket is readable then handle
			// it by accepting the connection
			if (FD_ISSET(listenfd, &rset)) 
			{
				printf("Signal from TCP client catched.\n");

				len = sizeof(client_addr);
				connfd = accept(listenfd, (struct sockaddr*)&client_addr, &len);
				
				if (connfd < 0) {
					printf("accept(): %s\n", strerror(errno));
					close(connfd);
					continue;						
				}

				// create the thread for each connection
				// if ((err = pthread_create(&tid, NULL, sendFile, (void *)&connfd)) != 0)
				// 	printf("can't create thread.\n");

				// without thread
				sending_file(connfd);
			}

			// if udp socket is readable receive the message
			if (FD_ISSET(udpfd, &rset)) 
			{
				printf("Signal from UDP client catched.\n");

				len = sizeof(client_addr);
				bzero(bufferFname, sizeof(bufferFname));

				printf("Message from UDP client.\n");
				n = recvfrom(udpfd, bufferFname, sizeof(bufferFname), 0, 
					(struct sockaddr*)&client_addr, &len);

				puts(bufferFname);

				// checking file name from client
				FILE *fp;
				if ((fp = fopen(bufferFname, "r")) == NULL)
					printf("File does not exist.\n");


				fseek(fp, 0, SEEK_END);
				size_t file_size = ftell(fp);

				while (bytes > 0) 
				{ 	
					unsigned char bufferSendData[NUM_BYTES_CHUNK] = {0};

					if ((fread(bufferSendData, ONE_BYTE, NUM_BYTES_CHUNK, fp)) < 0) {
						printf("Unable to copy file into buffer\n");
						break;
					}

					// reply to client
					bytes = sendto(udpfd, bufferSendData, sizeof(bufferSendData), 0,
						(struct sockaddr*)&client_addr, sizeof(client_addr));

					if (bytes == -1) {
						printf("error %s\n", strerror(errno));
						break;												
					} 

				}

				
			}

		}
	}

	printf("Closing both of socket ...\n");
	close(listenfd);
	close(udpfd);

	printf("Communication: FINISHED.\n");
}