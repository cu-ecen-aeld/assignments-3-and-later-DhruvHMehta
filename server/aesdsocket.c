/************************************************************************
*
*	AESD SOCKET PROGRAM
*	Brief: Program to accept multiple connections over port 9000.
*
*	Code References: https://beej.us/guide/bgnet/html/
*	Beej's Guide to Network Programming
*
*	https://github.com/cu-ecen-aeld/aesd-lectures/blob/master/lecture9/timer_thread.c
*	Timer Thread Example Code from Aesd-assignments repo
*
*	https://github.com/stockrt/queue.h/blob/master/sample.c
*	Used for understanding the usage of Singly Linked List from queue.h
*
*************************************************************************/
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
#include <stdbool.h>
#include <pthread.h>
#include <sys/queue.h>
#include <time.h>
#include <signal.h>

#define PORT 		"9000"
#define BACKLOG		5
#define BUF_SIZE	100
#define FILE_PATH	"/var/tmp/aesdsocketdata"

/* Socket File descriptor */
int socket_fd;
bool quitpgm = 0;
pthread_mutex_t file_mutex;

struct threadp
{
	pthread_t thread;
	int t_thread_id;
	int t_client_fd;
	int t_data_file;
	char t_IP[20];
	bool t_is_complete;
};

struct slist_data_s
{
	struct threadp t_param;
	SLIST_ENTRY(slist_data_s) entries;
};

struct timethreadp
{
	int data_file;
};

static inline void timespec_add( struct timespec *result,
                        const struct timespec *ts_1, const struct timespec *ts_2)
{
    result->tv_sec = ts_1->tv_sec + ts_2->tv_sec;
    result->tv_nsec = ts_1->tv_nsec + ts_2->tv_nsec;
    if( result->tv_nsec > 1000000000L ) {
        result->tv_nsec -= 1000000000L;
        result->tv_sec ++;
    }
}

static void sighandler(int signo)
{
	if((signo == SIGINT) || (signo == SIGTERM))
	{
		syslog(LOG_DEBUG, "Caught signal, exiting\n");
		if(shutdown(socket_fd, SHUT_RDWR))
		{
			perror("Failed on shutdown()");
		}
		quitpgm = 1;
	}

}

static void timer_thread(union sigval sigval)
{
	int str_byte;
	char *str = malloc(BUF_SIZE*sizeof(char));
	if(str == NULL)
	{
		perror("Malloc failed\n");
	//	pthread_exit(NULL);
	}

	struct timethreadp *td = (struct timethreadp *)sigval.sival_ptr;

	time_t cur_time = time(NULL);
	
	/* Get time in broken-down struct */
	struct tm *br_time = localtime(&cur_time);

	/* Get the formatted string as per requirements in str */	
	if((str_byte = strftime(str, BUF_SIZE, "timestamp:%Y %b %a %d %H:%M:%S%n", br_time)) == 0)
	{
		perror("strftime failed\n");
		free(str);
	//	pthread_exit(NULL);
	}	

	if(pthread_mutex_lock(&file_mutex) != 0)
	{
		perror("Mutex lock failed\n");
		free(str);
	//	pthread_exit(NULL);
	}

	/* Write the timestamp to file */
	int t_wr_bytes = write(td->data_file, str, str_byte);
	if(t_wr_bytes != str_byte)
		syslog(LOG_ERR,"error in writing timestamp to file\n");
	
	if(pthread_mutex_unlock(&file_mutex) != 0)
	{
		perror("Mutex unlock failed\n");
		free(str);
	//	pthread_exit(NULL);
	}
	
	free(str);

}

void* TxRxData(void *thread_param)
{
	struct threadp *l_threadp = (struct threadp*) thread_param;
	int  recv_bytes, fread_bytes;
	sigset_t new_set, old_set;
	int bufloc = 0, wrbufloc = 0;
	int bufcount = 1, wrbufcount = 1;
	char rd_byte;
	char *rxbuf;
	char *txbuf;
	int rc;

	syslog(LOG_DEBUG, "In thread %d with clientfd %d\n", l_threadp->t_thread_id, \
	l_threadp->t_client_fd);

	/* Malloc for recieve buffer from recv and transmit buffer for send */
	if((rxbuf = (char *)malloc(BUF_SIZE*sizeof(char))) == NULL)
	{
		perror("malloc failed");
		//pthread_exit(l_threadp);
	}	

	if((txbuf = (char *)malloc(BUF_SIZE*sizeof(char))) == NULL)
	{
		perror("malloc failed");
		free(rxbuf);
		//pthread_exit(l_threadp);
	}

	/* For Signal Masking during recv and send */
	sigemptyset(&new_set);
	sigaddset(&new_set, SIGINT);
	sigaddset(&new_set, SIGTERM);
	
	/* Don't accept a signal while recieving data */
	if((rc = sigprocmask(SIG_BLOCK, &new_set, &old_set)) == -1)
		printf("sigprocmask failed\n");
	
	/* Initially, write to starting of buffer */
	bufloc = 0;	
	
	/* Receive data */
	while((recv_bytes = recv(l_threadp->t_client_fd, rxbuf + bufloc, BUF_SIZE, 0)) > 0)
	{
		/* Error occurred in recv, log error */
		if(recv_bytes == -1)
		{
			printf("recv failed, %s\n", strerror(errno));
			free(rxbuf);
			free(txbuf);
			//pthread_exit(l_threadp);
		}

		/* Detect newline character */
		char* newlineloc = strchr(rxbuf, '\n');

		if(newlineloc != NULL)
		{
			/* Find the array index of the newline */
			int pos = newlineloc - (rxbuf + bufloc);

			/* If the newline is found outside the buffer bounds,
			 * ignore it   */
			if(pos < BUF_SIZE)
			{
				//syslog(LOG_DEBUG, "pos = %d, bufloc = %d\n", pos, bufloc);
				recv_bytes = pos + 1;
			}
		}
	
		bufloc = bufloc + recv_bytes;

		/* Buffer size needs to be increased */
		if((bufloc) >= (bufcount * BUF_SIZE))
		{
			bufcount++;
			char* newptr = realloc(rxbuf, bufcount*BUF_SIZE*sizeof(char));

			if(newptr == NULL)
			{
				free(rxbuf);
				free(txbuf);
				printf("Reallocation failed\n");
				//pthread_exit(l_threadp);
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
	pthread_mutex_lock(&file_mutex);
	int wr_bytes = write(l_threadp->t_data_file, rxbuf, bufloc);
	pthread_mutex_unlock(&file_mutex);

	if(wr_bytes != bufloc)
		syslog(LOG_ERR, "Bytes written to file do not match bytes recieved from connection\n");

	if((pthread_mutex_lock(&file_mutex)) != 0)
	{
		perror("Mutex lock failed\n");
		free(rxbuf);
		free(txbuf);
		//pthread_exit(l_threadp);
	}
	/* Set position of file pointer to start for reading */
	lseek(l_threadp->t_data_file, 0, SEEK_SET);

	/* Set buffer index to start of the buffer and read byte-by-byte */
	wrbufloc = 0;
	while((fread_bytes = read(l_threadp->t_data_file, &rd_byte, sizeof(char))) > 0)
	{
		if(fread_bytes == -1)
		{
			perror("read failed\n");
			free(rxbuf);
			free(txbuf);
			//pthread_exit(l_threadp);
		}

		if((pthread_mutex_unlock(&file_mutex) != 0))
		{
			perror("Mutex unlock failed\n");
			free(rxbuf);
			free(txbuf);
			//pthread_exit(l_threadp);
		}	

		/* Add a byte to the string */
		txbuf[wrbufloc] = rd_byte;

		/* Check if newline */
		if(txbuf[wrbufloc] == '\n')
		{	
			/* Mask off signals while sending data */
			if((rc = sigprocmask(SIG_BLOCK, &new_set, &old_set)) == -1)
				printf("sigprocmask failed\n");
			
			/* Send data read from file to client */
			int sent_bytes = send(l_threadp->t_client_fd, (txbuf), (wrbufloc + 1), 0);

			/* Unmask signals after data is sent */
			if((rc = sigprocmask(SIG_UNBLOCK, &old_set, NULL)) == -1)
				printf("sigprocmask failed\n");
		
			/* Error in sending */
			if(sent_bytes == -1)
			{
				perror("send failed\n");
				free(rxbuf);
				free(txbuf);
				//pthread_exit(l_threadp);
			}

			/* wrbufloc has to start at 0 at the line below */
			wrbufloc = -1;
		}

		/* Next array index */
		wrbufloc++;

		/* No space on the current buffer, realloc */
		if(wrbufloc == (wrbufcount * BUF_SIZE))
		{	wrbufcount++;
			char* newptr = realloc(txbuf, wrbufcount*BUF_SIZE*sizeof(char));

			if(newptr == NULL)
			{
				free(rxbuf);
				free(txbuf);
				perror("Reallocation failed\n");
				//pthread_exit(l_threadp);
			}

			else txbuf = newptr;
		}

		if((pthread_mutex_lock(&file_mutex)) != 0)
		{
			perror("Mutex lock failed\n");
			free(rxbuf);
			free(txbuf);
			//pthread_exit(l_threadp);
		}
	
	}
	
	if((pthread_mutex_unlock(&file_mutex)) != 0)
	{
		perror("Mutex unlock failed\n");
		free(rxbuf);
		free(txbuf);
		//pthread_exit(l_threadp);
	}
	
	/* Close connection */
	close(l_threadp->t_client_fd);

	if(l_threadp->t_client_fd == -1)
	{
		free(rxbuf);
		free(txbuf);
		printf("close failed\n");
	//	pthread_exit(l_threadp);
	}

	syslog(LOG_DEBUG, "Closed connection from %s\n", l_threadp->t_IP);
	free(rxbuf);
	free(txbuf);

	l_threadp->t_is_complete = true; 
	pthread_exit(NULL);

} // TxRxThread end


int main(int argc, char* argv[])
{
	struct addrinfo *sockaddrinfo;
	struct addrinfo hints;
	struct sockaddr_in clientsockaddr;
	socklen_t addrsize = sizeof(struct sockaddr);
	int opt = 1;
	int startdaemon = 0;
	pid_t pid;
	int client_fd, data_file; 
	int threadid = 0;	
	struct slist_data_s *node = NULL;
   	struct sigevent sev;	
	timer_t timerid;
	int clock_id = CLOCK_MONOTONIC;
	struct timespec start_time;
	struct itimerspec itimerspec;	
	struct timethreadp ttp;

	memset(&hints, 0, sizeof(hints));
	memset(&sev,0,sizeof(struct sigevent));

	SLIST_HEAD(slisthead, slist_data_s) head;
	SLIST_INIT(&head);

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

	if(argc == 2)
	{
		if(strcmp("-d", argv[1]) == 0)
		{
			startdaemon = 1;
		}	
	}

  	/* Create socket endpoint */ 
	socket_fd = socket(PF_INET, SOCK_STREAM, 0);
	
	/* Socket creation failed, return -1 on error */
	if(socket_fd == -1)
	{
		printf("Socket creation failed\n");
		return -1;
	}

	 /* Forcefully attaching socket to the port 9000 for bind error: address in use */
    	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    	{
       		 perror("setsockopt");
		 goto cleanexit;    
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
		goto cleanexit;
	}
	/* Bind */
	rc = bind(socket_fd, sockaddrinfo->ai_addr, sizeof(struct sockaddr));

	/* Error occurred in bind, return -1 on error */
	if(rc == -1)
	{
		printf("bind failed, %s\n", strerror(errno));
		freeaddrinfo(sockaddrinfo);
		goto cleanexit;
	}

	freeaddrinfo(sockaddrinfo);

	/* Listen for a connection */
	rc = listen(socket_fd, BACKLOG);

	/* Error occurred in listen, return -1 on error */
	if(rc == -1)
	{
		perror("listen failed\n");
		goto cleanexit;
	}

	/* Open file for writing */
	data_file = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, S_IRWXU);

	if(data_file == -1)
	{
		perror("open failed\n");
		goto cleanexit;
	}

	/* Initialize mutex */
	pthread_mutex_init(&file_mutex, NULL);	

	if(startdaemon)
	{
		pid = fork();
		if(pid == -1)
			goto cleanexit;

		else if(pid != 0)
			exit(0);

		if(setsid() == -1)
			goto cleanexit;

		if(chdir("/") == -1)
			goto cleanexit;

		open("/dev/null", O_RDWR);
		dup(0);
		dup(0);

	}

	if((startdaemon == 0) || (pid == 0))
	{
		/* Code referenced from link in File Header
		 * Code to run a thread on expiration of 
		 *  a timer  */
		ttp.data_file = data_file;

		sev.sigev_notify = SIGEV_THREAD;
		sev.sigev_value.sival_ptr = &ttp;
		sev.sigev_notify_function = timer_thread;

		/* Create per-process timer with MONOTONIC Clock */
		if ( timer_create(clock_id, &sev, &timerid) != 0 ) 
		{
		    printf("Error %d (%s) creating timer!\n", errno, strerror(errno));
		}

		/* Get current time */
		if ( clock_gettime(clock_id, &start_time) != 0 )
		{
			printf("Error %d (%s) getting clock %d time\n",errno,strerror(errno),clock_id);
		}

		/* Set timer expiration to 10s + 1mS (to make sure system tick is complete) */
		itimerspec.it_interval.tv_sec = 10;
		itimerspec.it_interval.tv_nsec = 1000000;

		timespec_add(&itimerspec.it_value, &start_time, &itimerspec.it_interval);

		/* Arm the timer for the absolute time specified in itimerspec */	
		if( timer_settime(timerid, TIMER_ABSTIME, &itimerspec, NULL ) != 0 ) 
		{
		    printf("Error %d (%s) setting timer\n",errno,strerror(errno));
		}
	}
	
	while(!quitpgm)
	{		

		/* Get the accepted client fd */
		client_fd = accept(socket_fd, (struct sockaddr *)&clientsockaddr, &addrsize); 

		if(quitpgm)
			break;
		
		/* Error occurred in accept, return -1 on error */
		if(client_fd == -1)
		{
			perror("accept failed\n");
			goto cleanexit;
		}
		
		/* sockaddr to IP string */
		char *IP = inet_ntoa(clientsockaddr.sin_addr);
		/* Log connection IP */
		syslog(LOG_DEBUG, "Accepted connection from %s\n", IP);

		/* Fill thread paramters and create new thread */
		threadid++;
		node = malloc(sizeof(struct slist_data_s));
		(node->t_param).t_thread_id = threadid;
		(node->t_param).t_client_fd = client_fd;
		(node->t_param).t_data_file = data_file;
		strcpy((node->t_param).t_IP, IP);
		(node->t_param).t_is_complete = false;
		SLIST_INSERT_HEAD(&head, node, entries);

		pthread_create(&((node->t_param).thread), NULL, &TxRxData, &(node->t_param));

		SLIST_FOREACH(node, &head, entries)
		{
			if((node->t_param).t_is_complete == true)
			{
				pthread_join((node->t_param).thread, NULL);	
			}
		}
	}

cleanexit:	
	close(data_file);
	close(client_fd);
	close(socket_fd);
	remove(FILE_PATH);

	 while (!SLIST_EMPTY(&head))
 	{
		node = SLIST_FIRST(&head);
		SLIST_REMOVE_HEAD(&head, entries);
		free(node);
    	}
	
	return 0;

} // Main end
