#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define PORT 		"9000"
#define BACKLOG		5
#define BUF_SIZE	100
#define FILE_PATH	"/var/tmp/aesdsocketdata"

/* File descriptor to write received data */
int data_file;
int socket_fd, client_fd;

static void sighandler(int signo)
{
	if((signo == SIGINT) || (signo == SIGTERM))
	{
		syslog(LOG_DEBUG, "Caught signal, exiting\n");
		remove(FILE_PATH);
		close(data_file);
		close(client_fd);
		close(socket_fd);
		exit(0);
	}

}

int main()
{
	struct addrinfo hints;
	struct addrinfo *sockaddrinfo;
	struct sockaddr_in clientsockaddr;
	socklen_t addrsize = sizeof(struct sockaddr);
	int  recv_bytes;
	char rxbuf[BUF_SIZE];
	char txbuf[BUF_SIZE];
	sigset_t new_set, old_set;
//	int bufloc = 0;
//	int bufcount = 1;

	openlog(NULL, 0, LOG_USER);

	if(signal(SIGINT, sighandler) == SIG_ERR)
	{
		printf("Cannot handle SIGINT\n");
		return -1;
	}

	if(signal(SIGTERM, sighandler) == SIG_ERR)
	{
		printf("Cannot handle SIGTERM\n");
		return -1;
	}

  	/* Create socket endpoint */ 
	socket_fd = socket(PF_INET, SOCK_STREAM, 0);
	
	/* Socket creation failed, return -1 on error */
	if(socket_fd == -1)
	{
		printf("Socket creation failed\n");
		return -1;
	}

	/* Setting this for use with getaddrinfo for bind() */
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int rc = getaddrinfo(NULL, PORT, &hints, &sockaddrinfo);

	/* Error occurred in getaddrinfo, return -1 on error */
	if(rc != 0)
	{
		printf("getaddrinfo failed, %s\n", gai_strerror(rc));
		freeaddrinfo(sockaddrinfo);
		return -1;
	}

	/* Bind */
	rc = bind(socket_fd, sockaddrinfo->ai_addr, sizeof(struct sockaddr));

	/* Error occurred in bind, return -1 on error */
	if(rc == -1)
	{
		printf("bind failed, %s\n", strerror(errno));
		freeaddrinfo(sockaddrinfo);
		return -1;
	}

	freeaddrinfo(sockaddrinfo);

	/* Listen for a connection */
	rc = listen(socket_fd, BACKLOG);

	/* Error occurred in listen, return -1 on error */
	if(rc == -1)
	{
		printf("listen failed\n");
		return -1;
	}

	/* Open file for writing */
	data_file = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, S_IRWXU);

	if(data_file == -1)
	{
		printf("open failed\n");
		return -1;
	}
	sigemptyset(&new_set);
	sigaddset(&new_set, SIGINT);
	sigaddset(&new_set, SIGTERM);
	
//	if((rxbuf = (char *)malloc(BUF_SIZE*sizeof(char))) == NULL)
//		printf("malloc failed");
		
	while(1)
	{
		/* Get the accepted client fd */
		client_fd = accept(socket_fd, (struct sockaddr *)&clientsockaddr, &addrsize); 

		/* Error occurred in accept, return -1 on error */
		if(client_fd == -1)
		{
			printf("accept failed\n");
			//free(rxbuf);
			return -1;
		}

		/* sockaddr to IP string */
		char *IP = inet_ntoa(clientsockaddr.sin_addr);
		/* Log connection IP */
		syslog(LOG_DEBUG, "Accepted connection from %s\n", IP);

		if((rc = sigprocmask(SIG_BLOCK, &new_set, &old_set)) == -1)
			printf("sigprocmask failed\n");
			
		/* Receive data */
		recv_bytes = recv(client_fd, rxbuf, BUF_SIZE - 1, 0);
	
		if((rc = sigprocmask(SIG_UNBLOCK, &old_set, NULL)) == -1)
			printf("sigprocmask failed\n");
		

/*
		while((recv_bytes = recv(client_fd, rxbuf + bufloc, BUF_SIZE - 1, 0)) > 0)
		{
			/ Error occurred in recv, log error /
			if(recv_bytes == -1)
			{
				printf("recv failed, %s\n", strerror(errno));
				return -1;
			}
			
			bufloc = bufloc + recv_bytes;

			/ Buffer size needs to be increased /
			if((bufloc) > (BUF_SIZE - 1))
			{
				bufcount++;
				char* newptr = realloc(rxbuf, bufcount*BUF_SIZE*sizeof(char));

				if(newptr == NULL)
				{
					free(rxbuf);
					printf("Reallocation failed\n");
				}

				else rxbuf = newptr;
					
			}
		}
*/		
		printf("Size of data = %d\n", recv_bytes);
		rxbuf[recv_bytes] = '\0';

		if(recv_bytes > 0)
			printf("Received data is %s\n", rxbuf);

		/* Write to file */
		int wr_bytes = write(data_file, rxbuf, recv_bytes);

		if(wr_bytes != recv_bytes)
			printf("did not write completely remove this line\n");
		
		lseek(data_file, 0, SEEK_SET);
		int fread_bytes = read(data_file, txbuf, BUF_SIZE);

		printf("txbuf is %s\n", txbuf);

		if(fread_bytes == -1)
		{
			printf("read failed\n");
			return -1;
		}

		if((rc = sigprocmask(SIG_BLOCK, &new_set, &old_set)) == -1)
			printf("sigprocmask failed\n");
			
		/* Send data read from file to client */
		int sent_bytes = send(client_fd, txbuf, fread_bytes, 0);
		
		if((rc = sigprocmask(SIG_UNBLOCK, &old_set, NULL)) == -1)
			printf("sigprocmask failed\n");
		
		if(sent_bytes > 0)
			printf("Sent data is %s\n", txbuf);

		if(sent_bytes == -1)
		{
			printf("send failed\n");
			return -1;
		}

		/* Close connection */
		close(client_fd);

		if(client_fd == -1)
		{
			printf("close failed\n");
			return -1;
		}

		syslog(LOG_DEBUG, "Closed connection from %s\n", IP);

	}


	return 0;
}
