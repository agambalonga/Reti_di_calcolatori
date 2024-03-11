#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>


void build_server_address(struct sockaddr_in *, int);
int add_exam(char, char);

/*
    1)Riceve l'aggiunta di nuovi esami
    2)Riceve la prenotazione di un esame
*/
int main(int argc, char *argv[]) {

    //create server socket
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    //handle error creating server socket
    if(!server_socket) {
        perror("Error creating server socket");
        exit(-1);
    }

    //define server address
    struct sockaddr_in server_address;
    build_server_address(&server_address, 9002);

    //bind server address to server socket
    bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));

    //listen for connections
    //first parameter is the socket, second is the maximum number of pending connections
    if(listen(server_socket, 5) < 0) {
        perror("Error listening for connections");
        exit(-2);
    }

    while(1) {
        /* accept new client connection. 
         * It returns a new socket file descriptor that is used to communicate with the client and 
         * the old one is used to listen for new connections
        */
        int client_socket = accept(server_socket, NULL, NULL);
        if(client_socket < 0) {
            perror("Error accepting connection");
        }

        pid_t pid = fork();
        if(pid < 0) {
            perror("Error creating child process");
        } else if (pid == 0) {
            //child process
            close(server_socket);

            //receive client message
            char buffer[256];
            recv(client_socket, buffer, sizeof(buffer), 0);

            /*
             * Example of requests:
             * 0, Reti di calcolatori, 2024/03/01 --> add new exam
             * 1, Reti di calcolatori, 0124002583, 2024/03/01 --> book exam
             * 2, Reti di calcolatori --> get dates by exam
            */
            char *token = strtok(buffer, ",");
            int operation = atoi(token);

            if(operation == 0) {
                //aggiungi esame
                char exam = strtok(NULL, ",");
                char date = strtok(NULL, ",");
        
                int result = add_exam(exam, date);
                if(result < 0) {
                    //error adding exam
                    send(client_socket, "Error adding exam", sizeof("Error adding exam"), 0);
                } else {
                    send(client_socket, "Exam added", sizeof("Exam added"), 0);
                }
            } else if (operation == 1) {
                //prenota esame
            } else if (operation == 2) {
                //get dates by exam
            } else {
                //invalid operation
                send(client_socket, "Invalid operation", sizeof("Invalid operation"), 0);
            }
            
        } else {
            //parent process
            close(client_socket);

        }
    }

    return 0;
}


void build_server_address(struct sockaddr_in *server_address, int port) {
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(port);
    server_address->sin_addr.s_addr = INADDR_ANY;
}


/*
    Add exam into a file only if not already present for that date
    Return 0 if exam added, -1 if exam already present
*/
int add_exam(char exam, char date) {

    //open file in read and append mode
    FILE *file = fopen("exams.txt", "a+");
    if(file == NULL) {
        perror("Error opening file");
        return -1;
    }

    //check if exam is already present
    char line[256];
    while(fgets(line, sizeof(line), file)) {
        // check if exam is already present. A line is in the format: exam,date
        char *token = strtok(line, ",");
        if(strcmp(token, exam) == 0 && strcmp(strtok(NULL, ","), date) == 0) {
            //exam already present
            return -2;
        }
    }

    //add exam
    fprintf(file, "%s,%s\n", exam, date);
    fclose(file);
    return 0;
}