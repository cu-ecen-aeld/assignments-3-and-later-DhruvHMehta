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
char *rxbuf;
char *txbuf;
static void sighandler(int signo)
{
	if((signo == SIGINT) || (signo == SIGTERM))
	{
		syslog(LOG_DEBUG, "Caught signal, exiting\n");
		remove(FILE_PATH);
		close(data_file);
		close(client_fd);
		close(socket_fd);
		free(rxbuf);
		free(txbuf);
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
	sigset_t new_set, old_set;
	int bufloc = 0;
	int bufcount = 1;
	off_t filesize;

	openlog(NULL, 0, LOG_USER);

	/* Setting up signal handlers SIGINT and SIGTERM */
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

	/* For Signal Masking during recv and send */
	sigemptyset(&new_set);
	sigaddset(&new_set, SIGINT);
	sigaddset(&new_set, SIGTERM);
	
	/* Malloc for recieve buffer from recv and transmit buffer for send */
	if((rxbuf = (char *)malloc(BUF_SIZE*sizeof(char))) == NULL)
	{
		printf("malloc failed");
		return -1;
	}	

	if((txbuf = (char *)malloc(BUF_SIZE*sizeof(char))) == NULL)
	{
		printf("malloc failed");
		return -1;
	}
	
	while(1)
	{
		/* Get the accepted client fd */
		client_fd = accept(socket_fd, (struct sockaddr *)&clientsockaddr, &addrsize); 

		/* Error occurred in accept, return -1 on error */
		if(client_fd == -1)
		{
			printf("accept failed\n");
			free(rxbuf);
			return -1;
		}

		/* sockaddr to IP string */
		char *IP = inet_ntoa(clientsockaddr.sin_addr);
		/* Log connection IP */
		syslog(LOG_DEBUG, "Accepted connection from %s\n", IP);

		/* Don't accept a signal while recieving data */
		if((rc = sigprocmask(SIG_BLOCK, &new_set, &old_set)) == -1)
			printf("sigprocmask failed\n");
		
		/* Initially, write to starting of buffer */
		bufloc = 0;	
		
		/* Receive data */
		while((recv_bytes = recv(client_fd, rxbuf + bufloc, BUF_SIZE, 0)) > 0)
		{
			/* Error occurred in recv, log error */
			if(recv_bytes == -1)
			{
				printf("recv failed, %s\n", strerror(errno));
				return -1;
			}

			/* Detect newline character */
			char* newlineloc = strchr(rxbuf, '\n');

			bufloc = bufloc + recv_bytes;

			/* Buffer size needs to be increased */
			if((bufloc) >= (bufcount * BUF_SIZE))
			{
				bufcount++;
				char* newptr = realloc(rxbuf, bufcount*BUF_SIZE*sizeof(char));

				if(newptr == NULL)
				{
					free(rxbuf);
					printf("Reallocation failed\n");
					return -1;
				}

				else rxbuf = newptr;
					
			}

			/* Newline exists, break out here */
			else if (newlineloc != NULL)
			       	break;
			
		}
		
		/* Unmask pending signals */
		if((rc = sigprocmask(SIG_UNBLOCK, &old_set, NULL)) == -1)
			printf("sigprocmask failed\n");
		
		/* Write to file */
		int wr_bytes = write(data_file, rxbuf, bufloc);

		if(wr_bytes != bufloc)
			syslog(LOG_ERR, "Bytes written to file do not match bytes recieved from connection\n");
	
		/* Find file size for reallocating write buffer */
		filesize = lseek(data_file, 0, SEEK_CUR);
		
		char* newptr = realloc(txbuf, filesize*sizeof(char));
		if(newptr == NULL)
		{
			free(txbuf);
			printf("Reallocation failed\n");
			return -1;
		}

		else txbuf = newptr;

		/* Set position of file pointer to start for reading */
		lseek(data_file, 0, SEEK_SET);
		int fread_bytes = read(data_file, txbuf, filesize);

		if(fread_bytes == -1)
		{
			printf("read failed\n");
			return -1;
		}

		/* Mask off signals while sending data */
		if((rc = sigprocmask(SIG_BLOCK, &new_set, &old_set)) == -1)
			printf("sigprocmask failed\n");
			
		/* Send data read from file to client */
		int sent_bytes = send(client_fd, txbuf, fread_bytes, 0);
		
		/* Unmask signals after data is sent */
		if((rc = sigprocmask(SIG_UNBLOCK, &old_set, NULL)) == -1)
			printf("sigprocmask failed\n");
		
		/* Error in sending */
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
