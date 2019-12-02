#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

void gotoxy(int x, int y)
{
	printf("%c[%d;%df",0x1B, y, x);
}

int main(int argc, char *argv[])
{
	system("clear");
	int sockfd = 0;
	int bytesReceived = 0;

	char recvBuffer[1024];
	memset(recvBuffer, '0', sizeof(recvBuffer));

	struct sockaddr_in serv_addr;

	// create a socket first
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\nError: could not create socket");
		return 1;
	}

	// initialize sockaddr_in data structure
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5132); // port number
	char ip[50];

	if (argc < 2) 
	{
		printf("Enter ip address to connect: ");
		fgets(ip, sizeof(ip), stdin);
	} else strcpy(ip, argv[1]); 

	serv_addr.sin_addr.s_addr = inet_addr(ip);

	// attempt a connection 
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nError: Connect Failed \n");
		return 1;
	}

	// printf("Connected to ip: %s : %d\n", inet_ntoa(serv_addr.sin_addr.s_addr), ntohs(serv_addr.sin_port));

	/* Create file where data will be stored */
	FILE *fp;
	char fname[100];
	// unsigned long fsize=0;
	// char fsz[20];
	
	read(sockfd, fname, 256);
	// read(sockfd, fsz, 20);

	printf("File name: %s\n", fname);
	// printf("File size: %s bytes\n", fsz);

	// char *ps, *ptr;
	// ps = fsz;

	// fsize = strtol(ps, &ptr, 10);
	// long int doter = (long)fsize/100;

	printf("Receiving file ...");

	fp = fopen(fname, "ab");
	if (fp == NULL) 
	{
		printf("Error opening file\n");
		return -1;
	}

	long double sz = 1;
	int i = 0;

	while ((bytesReceived = read(sockfd, recvBuffer, 1024)) > 0)
	{
		sz ++;
		gotoxy(0,5);
		printf("Received: \033[0;32m %llf Mb\033[0m",(sz/1024));
		gotoxy(0,6);

		// unsigned long int per=(unsigned long)sz*1024;
		// float pers=((float)per/(float)fsize)*100;
		// printf("completed: [%0.2f%]  \n", pers);

		fflush(stdout);
		fwrite(recvBuffer, 1, bytesReceived, fp);

	}

	if (bytesReceived < 0)
		printf("Read error ...\n");

	printf("File OK .... completed\n");

	return 0;
}
