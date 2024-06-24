#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

#define SECRETARY_PORT 9003

int createSocket(struct sockaddr_in* , int);

ssize_t writeWithRetry(int, const void *, size_t, int);

int connectWithRetry(int, const struct sockaddr*, int);

void book_exam_request(int, char *, char *, char *date);

void get_exam_dates_request(int, char *);

int main(int argc, char *argv[]) {
    srand(time(NULL));

    char response[256];
    char buffer[256];
    int choice=1;

    // Segretary address
    struct sockaddr_in segretary_address;

    // Create socket for student
    int segretary_socket = createSocket(&segretary_address, SECRETARY_PORT);
    if (segretary_socket < 0) {
        perror("Error creating student socket");
        exit(-1);
    }

    // Connect to segretary
    if (connect(segretary_socket, (struct sockaddr *) &segretary_address, sizeof(segretary_address)) < 0) {
        if (connectWithRetry(segretary_socket, (struct sockaddr *) &segretary_address, 3) < 0) {
            printf("Error connecting to segretary");
            exit(-1);
        }
    }

    while(choice != 3){

        memset(buffer, 0, sizeof(buffer));
        memset(response, 0, sizeof(response));

        printf("Which operation do you want do?\n1)View available exams\n2)Book exam\n3)Exit\nChoice:");
        scanf("%d", &choice);

        if(choice==1){
            printf("Insert the exam you want to view: ");
            scanf("%s", buffer);
            
            get_exam_dates_request(segretary_socket, buffer);
        }
        else if(choice==2){
            printf("Insert the exam you want to book (Exam name,Matricola,YYYY/MM/DD): ");
            scanf("%s", buffer);
            char *exam = strtok(buffer, ",");
            char *student_id = strtok(NULL, ",");
            char *date = strtok(NULL, ",");
            //check request format
            if (exam == NULL || student_id == NULL || date == NULL) {
                printf("Missing argument: Usage: <exam_name>,<student_id>,<exam_date>\n");
                exit(-1);
            }

            book_exam_request(segretary_socket, exam, student_id, date);
            
        }
        else if(choice!=3) {
            //invalid operation
            printf("Invalid operation\n");
        }
        printf("Waiting for response from server...\n");
        if (read(segretary_socket, response, sizeof(response)) <= 0) {
            printf("Error receiving response from server (timeout simulation)\n");
            
            //close socket and try to reconnect
            close(segretary_socket);
            segretary_socket = createSocket(&segretary_address, SECRETARY_PORT);
            
            if(connectWithRetry(segretary_socket, (struct sockaddr *) &segretary_address, 3) < 0) {
                printf("Error reconnetting to segretary server, exiting...");
                exit(-1);
            }
        }
        printf("Received response from server: %s\n", response);
    }
    printf("Application closed\n");
    close(segretary_socket);
    exit(0);

}

int createSocket(struct sockaddr_in *address, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &address->sin_addr);

    return sockfd;
}

ssize_t writeWithRetry(int fd, const void *buf, size_t count, int max_attempts) {
    int attempt=0;
    ssize_t result;
    while(attempt++ < max_attempts) {
        result = write(fd, buf, count);
        if (result != -1) {
            return result; // Write successful
        }
        printf("Error writing, retrying...");

        if(attempt == max_attempts) {
            int delay = rand()%6; // Random delay between 0 and 5 seconds
            printf("Wait: %d second\n", delay);
            sleep(delay); // Wait before retrying
            result = write(fd, buf, count);
            if (result != -1) {
                return result; // Write successful
            }
        }
    }
    return -1; // Write failed after max_attempts
}

int connectWithRetry(int sockfd, const struct sockaddr *addr, int max_attempts) {
    int attempt=1;
    while(attempt <= max_attempts) {
        printf("Connection attempt %d\n", attempt);

        if (connect(sockfd, addr, sizeof(*addr)) >= 0) {
            printf("Connection successful to segretary\n");
            return 0; // Connection successful
        }
        printf("Error connecting, retrying...\n");

        //once connect failed, the socket must be closed and recreated
        close(sockfd);
        sockfd = createSocket((struct sockaddr_in *) addr, SECRETARY_PORT);

        if(attempt == max_attempts) {
            int delay = rand()%11;// Random delay between 0 and 10 seconds
            printf("Wait: %d second\n", delay);
            sleep(delay); // Wait before retrying
            if (connect(sockfd, addr, sizeof(*addr)) >= 0) {
                printf("Connection successful to segretary\n");
                return 0; // Connection successful
            }
        }
        attempt++;
    }
    return -1; // Connection failed after max_attempts
}

void book_exam_request(int segretary_socket, char *exam, char *student_id, char *date) {
    exam[strcspn(exam, "\n")] = '\0';
    date[strcspn(date, "\n")] = '\0';
    student_id[strcspn(student_id, "\n")] = '\0';
    char request[256];
    sprintf(request, "1,%s,%s,%s", exam, student_id, date);
    printf("Sending book_exam_request to server...\n");
    if (write(segretary_socket, request, sizeof(request)) < 0) {
        perror("Error writing to segretary");
        exit(EXIT_FAILURE);
    }
}

void get_exam_dates_request(int segretary_socket, char *exam) {
    exam[strcspn(exam, "\n")] = '\0';
    char request[256];
    sprintf(request, "2,%s", exam);
    printf("Sending get_exam_dates_request to server...\n");
    if(write(segretary_socket, request, sizeof(request)) < 0) {
        perror("Error sending request to server");
        exit(EXIT_FAILURE);
    }
    
    printf("Request sent to server\n");
}