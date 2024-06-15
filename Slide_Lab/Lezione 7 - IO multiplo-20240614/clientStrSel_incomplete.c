#include <sys/types.h>   /* predefined types */
#include <unistd.h>      /* include unix standard library */
#include <arpa/inet.h>   /* IP addresses conversion utiliites */
#include <sys/socket.h>  /* socket library */
#include <stdio.h>	 /* include standard I/O library */
#include <errno.h>	 /* include error codes */
#include <string.h>	 /* include error strings definitions */

#define MAXLINE 256

#define max(x, y) ({typeof (x) x_ = (x); typeof (y) y_ = (y); \
x_ > y_ ? x_ : y_;}) 

void ClientEcho(FILE * filein, int socket);
ssize_t FullWrite(int fd, const void *buf, size_t count);

int main(int argc, char *argv[])
{
    int sock, i;
    int reset = 0;
    struct sockaddr_in serv_add;

    /* create socket */
    if ( (sock = socket(/*TODO: COMPLETE SOCKET*/)) < 0) {
		perror("Socket creation error");
		return 1;
    }
    
	/* initialize address */
    serv_add.sin_family = AF_INET;
    serv_add.sin_port = htons(1024);
    
	/* build address using inet_pton */
    if ( (inet_pton(/*TODO: COMPLETE SOCKET*/)) <= 0) {
		perror("Address creation error");
		return 1;
    }
	
    /* extablish connection */
    if (connect(/*TODO: COMPLETE SOCKET*/) < 0) {
		perror("Connection error");
		return 1;
    }
    
    /* do read/write operations */
    ClientEcho(stdin, sock);
    
	/* normal exit */
    return 0;
}

void ClientEcho(FILE * filein, int socket) 
{
    char sendbuff[MAXLINE+1], recvbuff[MAXLINE+1];
    int nread, nwrite; 
    int maxfd;
    fd_set fset;
	
    /* initialize file descriptor set */
	/*TODO: FD_ZERO() or FD_SET() or FD_ISSET()*/
    
	maxfd = max(fileno(filein), socket) + 1;
    while (1) {
	
		/* set for the socket */
		/*TODO: FD_ZERO() or FD_SET() or FD_ISSET()*/	

		/* set for the standard input */
		/*TODO: FD_ZERO() or FD_SET() or FD_ISSET()*/
		
		select(, , NULL, NULL, NULL); /* wait for read ready */
		
		/* if ready on stdin */
		if (/*TODO: FD_ZERO() or FD_SET() or FD_ISSET()*/) {
		    if (fgets(sendbuff, MAXLINE, filein) == NULL) { /* if no input */
				return;                /* we stopped client */
		    } else {                   /* else we have to write to socket */
			nwrite = FullWrite(/* TODO: COMPLETE WRITE */); 
				if (nwrite < 0) {      /* on error stop */
				    printf("Errore in scrittura: %s", strerror(errno));
				    return;
				}
		    }
		}
		
		/* if ready on socket */
		if (/*TODO: FD_ZERO() or FD_SET() or FD_ISSET()*/) { 
		    nread = read( /* TODO: COMPLETE READ */ ); /* do read */
		    if (nread < 0) {  /* error condition, stop client */
				printf("Errore in lettura: %s\n", strerror(errno));
				return;
		    }
		    if (nread == 0) { /* server closed connection, stop */
				printf("EOF sul socket\n");
				return;
		    }
		    recvbuff[nread] = 0;   /* else read is ok, write on stdout */
		    if (fputs(recvbuff, stdout) == EOF) {
				perror("Errore in scrittura su terminale");
				return;
		    }
		}
    }
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
