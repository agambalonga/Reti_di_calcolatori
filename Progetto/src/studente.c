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

#define SECRETARY_PORT 9003

ssize_t writeWithRetry(int fd, const void *buf, size_t count, int max_attempts) {
    int attempt=0;
    ssize_t result;
    while(attempt++ < max_attempts) {
        result = write(fd, buf, count);
        if (result != -1) {
            return result; // Write successful
        }
        perror("Error writing, retrying...");

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

int connectWithRetry(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int max_attempts) {
    int attempt=0;
    while(attempt++ < max_attempts) {
        if (connect(sockfd, addr, addrlen) == 0) {
            return 0; // Connection successful
        }
        perror("Error connecting, retrying...");
        if(attempt == max_attempts) {
            int delay = rand()%6;// Random delay between 0 and 5 seconds
            printf("Wait: %d second\n", delay);
            sleep(delay); // Wait before retrying
            if (connect(sockfd, addr, addrlen) == 0) {
                return 0; // Connection successful
            }
        }
        
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
    if (writeWithRetry(segretary_socket, request, sizeof(request), 3) < 0) {
        perror("Error writing to segretary");
        exit(EXIT_FAILURE);
    }
}

void get_exam_dates_request(int segretary_socket, char *exam) {
    exam[strcspn(exam, "\n")] = '\0';
    char request[256];
    sprintf(request, "2,%s", exam);
    printf("Sending get_exam_dates_request to server...\n");
    int n = writeWithRetry(segretary_socket, request, sizeof(request), 3);
    if (n < 0) {
        perror("Error sending request to server");
        exit(EXIT_FAILURE);
    }
    printf("Request sent to server\n");
}

int main(int argc, char *argv[]) {
    char response[256];
    char buffer[256];
    int choice=1;

    // Create socket for student
    int segretary_socket;
    segretary_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (segretary_socket < 0) {
        perror("Error creating student socket");
        exit(EXIT_FAILURE);
    }

    // Segretary address
    struct sockaddr_in segretary_address;
    segretary_address.sin_family = AF_INET;
    segretary_address.sin_port = htons(SECRETARY_PORT);
    segretary_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to segretary
    if (connect(segretary_socket, (struct sockaddr *) &segretary_address, sizeof(segretary_address)) < 0) {
        if (connectWithRetry(segretary_socket, (struct sockaddr *) &segretary_address, sizeof(segretary_address), 5) < 0) {
            perror("Error connecting to segretary");
            exit(EXIT_FAILURE);
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
        if (read(segretary_socket, response, sizeof(response)) < 0) {
            perror("Error receiving response from server");
            exit(EXIT_FAILURE);
        }
        printf("Received response from server: %s\n", response);
    }
    printf("Application closed\n");
    close(segretary_socket);
    exit(0);

}