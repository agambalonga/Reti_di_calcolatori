#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define SECRETARY_PORT 9003
#define SERVER_PORT 9002

void add_exam_request(int server_socket, char *exam, char *date) {
    exam[strcspn(exam, "\n")] = '\0';
    date[strcspn(date, "\n")] = '\0';
    char request[256];
    sprintf(request, "0,%s,%s", exam, date);
    send(server_socket, request, strlen(request), 0);
}

void book_exam_request(int server_socket, char *exam, char *student_id, char *date) {
    exam[strcspn(exam, "\n")] = '\0';
    date[strcspn(date, "\n")] = '\0';
    student_id[strcspn(student_id, "\n")] = '\0';
    char request[256];
    sprintf(request, "1,%s,%s,%s", exam, student_id, date);
    send(server_socket, request, strlen(request), 0);
}

void get_exam_dates_request(int server_socket, char *exam) {
    exam[strcspn(exam, "\n")] = '\0';
    char request[256];
    sprintf(request, "2,%s", exam);
    send(server_socket, request, strlen(request), 0);
}

int main(int argc, char *argv[]) {

    // Create socket for server connection
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // Server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    // Create socket for secretary
    int secretary_socket;
    secretary_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (secretary_socket < 0) {
        perror("Error creating secretary socket");
        exit(EXIT_FAILURE);
    }

    // Secretary address
    struct sockaddr_in secretary_address;
    secretary_address.sin_family = AF_INET;
    secretary_address.sin_port = htons(SECRETARY_PORT);
    secretary_address.sin_addr.s_addr = INADDR_ANY;

    // Bind secretary socket to address
    if (bind(secretary_socket, (struct sockaddr *) &secretary_address, sizeof(secretary_address)) < 0) {
        perror("Error binding secretary socket");
        exit(EXIT_FAILURE);
    }

    // Listen for connections from students
    if (listen(secretary_socket, 5) < 0) {
        perror("Error listening for secretary connections");
        exit(EXIT_FAILURE);
    }

    printf("Secretary listening on port %d\n", SECRETARY_PORT);

    while (1) {
        // Accept connection from student
        int student_socket;
        student_socket = accept(secretary_socket, NULL, NULL);
        if (student_socket < 0) {
            perror("Error accepting student connection");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Error creating child process");
        } else if (pid == 0) {
            // Child process - handle student request
            close(secretary_socket);

            //receive client message
            char buffer[256];
            recv(student_socket, buffer, sizeof(buffer), 0);
            /*
             * Example of requests:
             * 0,Reti di calcolatori,2024/03/01 --> add new exam
             * 1,Reti di calcolatori,0124002583, 2024/03/01 --> book exam
             * 2,Reti di calcolatori --> get dates by exam
            */
            char *token = strtok(buffer, ",");
            int operation = atoi(token);

            if (operation == 0) {
                char *exam = strtok(NULL, ",");
                char *date = strtok(NULL, ",");
                if (exam == NULL || date == NULL) {
                    printf("Usage: %d <exam_name> <exam_date>\n", operation);
                    return 1;
                }
                add_exam_request(server_socket, exam, date);
            }
            else if (operation == 1) {
                char *exam = strtok(NULL, ",");
                char *student_id = strtok(NULL, ",");
                char *date = strtok(NULL, ",");
                if (exam == NULL || date == NULL || student_id == NULL) {
                    printf("Usage: %d <exam_name> <exam_date>\n", operation);
                    return 1;
                }
                book_exam_request(server_socket, exam, student_id, date);
            }
            else if (operation == 2) {
                char *exam = strtok(NULL, ",");
                if (exam == NULL) {
                    printf("Usage: %d <exam_name>\n", operation);
                    return 1;
                }
                get_exam_dates_request(server_socket, exam);
            } else {
                printf("Invalid operation\n");
            }

            // Receive response from server
            char response[256];
            recv(server_socket, response, sizeof(response), 0);
            printf("Response from server: %s\n", response);
            send(student_socket, response, strlen(response), 0);

            // Close student socket
            close(student_socket);
            exit(0);
        } else {
            // Parent process - continue listening for other student connections
            close(student_socket);
        }
    }

    // Close sockets
    close(secretary_socket);
    close(server_socket);

    return 0;
}
