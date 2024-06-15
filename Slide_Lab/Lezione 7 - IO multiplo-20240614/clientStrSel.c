#include <sys/types.h>   /* predefined types */
#include <unistd.h>      /* include unix standard library */
#include <arpa/inet.h>   /* IP addresses conversion utiliites */
#include <sys/socket.h>  /* socket library */
#include <stdio.h>	 /* include standard I/O library */
#include <errno.h>	 /* include error codes */
#include <string.h>	 /* include erroro strings definitions */

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

    if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket creation error");
		return 1;
    }
    
    serv_add.sin_family = AF_INET;
    serv_add.sin_port = htons(1024);
    
    if ( (inet_pton(AF_INET, argv[1], &serv_add.sin_addr)) <= 0) {
		perror("Address creation error");
		return 1;
    }
	
    if (connect(sock, (struct sockaddr *)&serv_add, sizeof(serv_add)) < 0) {
		perror("Connection error");
		return 1;
    }
    
    ClientEcho(stdin, sock);

    return 0;
}

void ClientEcho(FILE * filein, int socket) 
{
    char sendbuff[MAXLINE+1], recvbuff[MAXLINE+1];
    int nread, nwrite; 
    int maxfd;
    fd_set fset;

    FD_ZERO(&fset);
    maxfd = max(fileno(filein), socket) + 1;

    while (1) {
	FD_SET(socket, &fset);
	FD_SET(fileno(filein), &fset);
	select(maxfd, &fset, NULL, NULL, NULL);
	if (FD_ISSET(fileno(filein), &fset)) {  
	    if (fgets(sendbuff, MAXLINE, filein) == NULL) { 
			return;                
	    } else {
			nwrite = FullWrite(socket, sendbuff, strlen(sendbuff)); 
			if (nwrite < 0) {
		   		printf("Errore in scrittura: %s", strerror(errno));
		    return;
		}
	    }
	}
	if (FD_ISSET(socket, &fset)) { 
	    nread = read(socket, recvbuff, strlen(sendbuff));
	    if (nread < 0) {  
			printf("Errore in lettura: %s\n", strerror(errno));
			return;
	    }
	    if (nread == 0) { 
			printf("EOF sul socket\n");
			return;
	    }
	    recvbuff[nread] = 0;
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
