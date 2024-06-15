#include <sys/types.h>   /* predefined types */
#include <unistd.h>      /* include unix standard library */
#include <arpa/inet.h>   /* IP addresses conversion utiliites */
#include <sys/socket.h>  /* socket library */
#include <stdio.h>	 /* include standard I/O library */
#include <time.h>
#include <syslog.h>      /* syslog system functions */
#include <signal.h>      /* signal functions */
#include <errno.h>       /* error code */
#include <string.h>      /* error strings */
#include <stdlib.h>

ssize_t FullWrite(int fd, const void *buf, size_t count);

#define BACKLOG 10
#define MAXLINE 256

/* Program beginning */
int main(int argc, char *argv[])
{
/* 
 * Variables definition  
 */
    int waiting = 0;
    int compat = 0;
    struct sockaddr_in s_addr, c_addr;
    socklen_t len;
    char buffer[MAXLINE];
    char fd_open[FD_SETSIZE];
    fd_set fset;
    int list_fd, fd;
    int max_fd, nread, nwrite;
    int i, n;

	/* create socket */
    if ( (list_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("Socket creation error");
	exit(1);
    }
    
	/* initialize address */
    memset((void *)&s_addr, 0, sizeof(s_addr));   /* clear server address */
    s_addr.sin_family = AF_INET;                  /* address type is INET */
    s_addr.sin_port = htons(1024);                   /* echo port is 7 */
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* connect from anywhere */
    
	/* bind socket */
    if (bind(list_fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0) {
	perror("bind error");
	exit(1);
    }
    
    /* main body */
    if (listen(list_fd, BACKLOG) < 0 ) {
		perror("listen error");
		exit(1);
    }
    
    /* initialize all needed variables */
    memset(fd_open, 0, FD_SETSIZE);   /* clear array of open files */
    max_fd = list_fd;                 /* maximum now is listening socket */
    fd_open[max_fd] = 1;
	
    /* main loop, wait for connection and data inside a select */
    while (1) {    
		/*TODO*/		/* clear fset */
		for (i = list_fd; i <= max_fd; i++) { /* initialize fd_set */
			if (fd_open[i] != 0)
				FD_SET(i, &fset); 
		}
		
		while ( ((n = select(,,,,)) < 0) && (errno == EINTR));         /* wait for data or connection */
		if (n < 0) {                          /* on real error exit */
			perror("select error");
			exit(1);
		}
		
		/* on activity */
		if (/*TODO*/) {       /* if new connection */
			n--;                              /* decrement active */
			len = sizeof(c_addr);
			if ((fd = /*TODO: accept new connection*/) < 0) {
				perror("accept error");
				exit(1);
			}
			fd_open[fd] = 1;                  /* set new connection socket */
			if (max_fd < fd)
				max_fd = fd;     /* if needed set new maximum */
		}
		
		/* loop on open connections */
		i = list_fd;                  /* first socket to look */
		while (n != 0) {              /* loop until active */
			i++;                      /* start after listening socket */
			if (fd_open[i] == 0)
				continue;   /* closed, go next */
			if (/*TO DO: if i-th descriptor is active*/) {
				n--;                         /* decrease active */
				nread = read( /* TODO: read from active socket MAXLINE into buffer*/);     /* read operations */
				if (nread < 0) {
					perror("Errore in lettura");
					exit(1);
				}
				if (nread == 0) {            /* if closed connection */
					/*TODO: close active socket*/;                /* close file */
					fd_open[i] = 0;          /* mark as closed in table */
					if (max_fd == i) {       /* if was the maximum */
						while (fd_open[--i] == 0);    /* loop down */
						max_fd = i;          /* set new maximum */
						break;               /* and go back to select */
					}
				continue;                /* continue loop on open */
				}
				buffer[nread]=0;
				/*TODO: compute string len*/
				/*TODO: FullWrite*/
				if (nwrite) {
					perror("Errore in scrittura");
					exit(1);
				}
			}
		}
	}
    /* normal exit, never reached */
	exit(0);
}

ssize_t FullWrite(int fd, const void *buf, size_t count)  
{ 
 size_t nleft; 
 ssize_t nwritten; 
 nleft = count; 
 while (nleft > 0) {             /* repeat until no left */ 
	if ( (nwritten = write(fd, buf, nleft)) < 0) { 
		if (errno == EINTR) {   /* if interrupted by system call */ 
			continue;           /* repeat the loop */ 
		} else { 
			return(nwritten);   /* otherwise exit with error */ 
		} 
	} 
	nleft -= nwritten;          /* set left to write */ 
	buf +=nwritten;             /* set pointer */ 
 } 
 return (nleft); 
}