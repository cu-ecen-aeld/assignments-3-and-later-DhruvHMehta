#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>

int main(int argc, char* argv[])
{
	openlog(NULL, 0, LOG_USER);

	/* Exit if number of arguments do not match writer requirements */
	if(argc != 3)
	{
		syslog(LOG_ERR, "Error: Please provide two arguments for the path-to-file and string to write\n");
		syslog(LOG_ERR, "Usage: ./writer [path-to-file] [string-to-write]\n");	
		exit(1);
	}

	char *path 	= argv[1];
	char *writestr	= argv[2];
	char openpath[200];

	/* Open file to write */
	int fd = open(path, O_CREAT | O_RDWR, S_IRWXU);

	if(fd == -1)
	{
		char *filename = basename(path);
		char *dir  = dirname(path); 
		char mdir[200];
		int openerr = errno;
		
		/* If directory does not exist, make it (including parent directories) */
		if(errno == ENOENT)
		{
			sprintf(mdir, "mkdir -p %s", dir);
			system(mdir); 

			strcpy(openpath, path);
			strcat(openpath, "/");
			
			fd = open(openpath, O_CREAT | O_RDWR, S_IRWXU);
			
			/* File could not be opened, quit program */
			if(fd == -1)
			{
				syslog(LOG_ERR, "open error = %s\n", strerror(openerr));
				exit(1);
			}
		}

		else
		{	/* Some other error, print it and exit program */
			syslog(LOG_ERR, "File could not be created, error = %s\n", strerror(openerr));
			exit(1);	
		}
	
	}

	/* Write to file */
	int byteswr = write(fd, writestr, strlen(writestr));

	/* Number of bytes written != -1 and == length of writestr */
	if(byteswr != strlen(writestr))
	{
		syslog(LOG_ERR, "File could not be written to, error = %s\n", strerror(errno));
	}

	syslog(LOG_DEBUG, "Writing %s to %s\n", writestr, openpath);
	
	/* Close file and exit */
	close(fd);
	if(fd == -1)
	{
		syslog(LOG_ERR, "File could not be closed, error = %s\n", strerror(errno));
	}

	return 0;
}
