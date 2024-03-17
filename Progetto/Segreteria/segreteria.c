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
#define SERVER_PORT 9002
#define ADD_EXAM_PORT 9004

void add_exam_request(int server_socket_uni, char *exam, char *date) {
    exam[strcspn(exam, "\n")] = '\0';
    date[strcspn(date, "\n")] = '\0';
    char request[256];
    sprintf(request, "0,%s,%s", exam, date);
    printf("Sending add_exam_request to server...\n");
    if (write(server_socket_uni, request, strlen(request)) < 0) {
        perror("Error sending request to server");
        exit(EXIT_FAILURE);
    }
}

void book_exam_request(int server_socket_uni, char *exam, char *student_id, char *date) {
    exam[strcspn(exam, "\n")] = '\0';
    date[strcspn(date, "\n")] = '\0';
    student_id[strcspn(student_id, "\n")] = '\0';
    char request[256];
    sprintf(request, "1,%s,%s,%s", exam, student_id, date);
    printf("Sending book_exam_request to server...\n");
    if (write(server_socket_uni, request, strlen(request)) < 0) {
        perror("Error sending request to server");
        exit(EXIT_FAILURE);
    }
}

void get_exam_dates_request(int server_socket_uni, char *exam) {
    exam[strcspn(exam, "\n")] = '\0';
    char request[256];
    sprintf(request, "2,%s", exam);
    printf("Sending get_exam_dates_request to server...\n");
    int n = write(server_socket_uni, request, strlen(request));
    printf("n: %d\n", n);
    if (n < 0) {
        perror("Error sending request to server");
        exit(EXIT_FAILURE);
    }
    printf("Request sent to server\n");
}

int main(int argc, char *argv[]) {

    // Server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

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
        char response[256];
        // Accept connection from student
        int client_socket;
        client_socket = accept(secretary_socket, NULL, NULL);
        if (client_socket < 0) {
            perror("Error accepting student connection");
        }
        
        pid_t pid = fork();
        if (pid < 0) {
            perror("Error creating secretary process");
        } else if (pid == 0) {

             // Create socket for server connection
            int server_socket_uni;
            server_socket_uni = socket(AF_INET, SOCK_STREAM, 0);
            if (server_socket_uni < 0) {
                perror("Error creating server socket");
                exit(EXIT_FAILURE);
            }

            // Connect to server
            if (connect(server_socket_uni, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
                perror("Error connecting to server");
                exit(EXIT_FAILURE);
            }

            // Child process - handle student request
            close(secretary_socket);
            
            // Generate a random wait time between 1 and 10 seconds
            srand(time(NULL));

            //receive client message
            char buffer[256];
            int attempts = 0;
            int max_attempts = 3;
            int wait_time;

            //Simulazione di un messaggio che potrebbe non arrivare
            TODO:Sostituire i printf con i write per il client
            for (int attempts = 0; attempts < max_attempts; attempts++) {
                printf("Waiting for client message...\n");
                ssize_t n = read(client_socket, buffer, sizeof(buffer));
                bool simulate_failure = attempts <= 2; // Simulate failure for the first 3 attempts
                if (simulate_failure) {
                    n = -1;
                }
                printf("read() returned %ld\n", n);
                if (n > 0) {
                    break;
                } else {
                    printf("read() failed, incrementing attempts\n");
                    printf("attempts is now %d\n", attempts + 1);
                    if (attempts + 1 == max_attempts) {
                        printf("Too many failed attempts, waiting before retrying\n");
                        wait_time = rand() % 60 + 1;
                        sleep(wait_time);
                    }
                }
            }

            /*
            * Example of requests:
            * 0,Reti di calcolatori,2024/03/01 --> add new exam
            * 1,Reti di calcolatori,0124002583, 2024/03/01 --> book exam
            * 2,Reti di calcolatori --> get dates by exam
            */
            char *token = strtok(buffer, ",");
            int operation = atoi(token);
            if (operation == 0) {
                //add new exam
                char *exam = strtok(NULL, ",");
                char *date = strtok(NULL, ",");

                //check request format
                if (exam == NULL || date == NULL) {
                    write(client_socket, "Missing argument: Usage: 0,<exam_name>,<exam_date>\n", sizeof("Missing argument: Usage: 0,<exam_name>,<exam_date>\n"));
                    exit(0);
                }

                add_exam_request(server_socket_uni, exam, date);
            } else if (operation == 1) {

                //book exam
                char *exam = strtok(NULL, ",");
                char *student_id = strtok(NULL, ",");
                char *date = strtok(NULL, ",");

                //check request format
                if (exam == NULL || student_id == NULL || date == NULL) {
                    write(client_socket, "Missing argument: Usage: 1,<exam_name>,<student_id>,<exam_date>\n", sizeof("Missing argument: Usage: 1,<exam_name>,<student_id>,<exam_date>\n"));
                    exit(0);
                }
                    
                book_exam_request(server_socket_uni, exam, student_id, date);
            } else if (operation == 2) {

                //get dates by exam
                char *exam = strtok(NULL, ",");

                //check request format
                if (exam == NULL) {
                    write(client_socket, "Missing argument: Usage: 2,<exam_name>\n", sizeof("Missing argument: Usage: 2,<exam_name>\n"));
                    exit(0);
                }

                get_exam_dates_request(server_socket_uni, exam);    
            } else {
                //invalid operation
                write(client_socket, "Invalid operation\n", sizeof("Invalid operation\n"));
            }

            printf("Waiting for response from server...\n");
            if (read(server_socket_uni, response, sizeof(response)) < 0) {
                perror("Error receiving response from server");
                exit(EXIT_FAILURE);
            }
            printf("Received response from server: %s\n", response);
            write(client_socket, response, strlen(response));

            close(server_socket_uni);
            exit(0);
             
        } else {
            // Close student socket
            close(client_socket);
        }
    }
    
    return 0;
}
