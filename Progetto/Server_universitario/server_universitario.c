#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define SERVER_PORT 9002

void build_server_address(struct sockaddr_in *, int);
int add_exam(char*, char*);
int book_exam(char*, char*, char*);
int get_exam_dates(char*, char*);

/*
    1)Riceve l'aggiunta di nuovi esami
    2)Riceve la prenotazione di un esame
    3)Visualizza esami
*/
int main(int argc, char *argv[]) {

    //create server socket
    int server_socket, client_socket = -1;

    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creating server socket");
        exit(-1);
    }

    //handle error creating server socket
    if(!server_socket) {
        perror("Error creating server socket");
        exit(-1);
    }

    //define server address
    struct sockaddr_in server_address;
    build_server_address(&server_address, SERVER_PORT);

    //bind server address to server socket
    if(bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("Error binding server address to server socket");
        exit(-2);
    }

     /**
     * Ci serviamo di un "insieme" di descrittori per strutturare la successiva select.
     * In particolare abbiamo read_set che mantiene l'insieme dei descrittori in lettura.
     * La variabile max_fd serve a specificare quante posizioni dell'array di descrittori devono essere controllate
     * all'interno della funzione select.
     */
    fd_set read_set;
    int max_fd;
    max_fd = server_socket;

    //listen for connections
    //first parameter is the socket, second is the maximum number of pending connections
    if(listen(server_socket, 5) < 0) {
        perror("Error listening for connections");
        exit(-2);
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    while(1) {

         /**
         * Ad ogni iterazione reinizializzo a 0 il read_set e vi aggiungo la socket che permette l'ascolto di nuove
         * connessioni da parte della segreteria.
         */
        FD_ZERO(&read_set);
        FD_SET(server_socket, &read_set);

        /**
         * Se il descrittore della socket relativa alla connessione con la segreteria è maggiore di -1 significa che la
         * segreteria è connessa e quindi si aggiunge anche il suo descrittore al read_set.
         */
        if (client_socket > -1) {
            FD_SET(client_socket, &read_set);
        }

        /**
         * La funzione select restituisce il numero di descrittori pronti.
         */
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            perror("Errore nell'operazione di select!");
        }
        
        /**
         * Si controlla se sono in attesa di essere accettate nuove connessioni.
         */
        if (FD_ISSET(server_socket, &read_set)) {
            /**
             * La system call accept permette di accettare una nuova connessione (lato server) in entrata da un client.
             */
            if ((client_socket = accept(server_socket, (struct sockaddr *)NULL, NULL)) < 0) {
                perror("Errore nell'operazione di accept!");
            }

            /**
             * Si ricalcola il numero di posizioni da controllare nella select
             */
            if (client_socket > max_fd) {
                max_fd = client_socket;
            }
        }

        /**
         * Si controlla se la segreteria vuole inviare una nuova richiesta al server universitario.
         */
        if (FD_ISSET(client_socket, &read_set)) {
            /**
             * In caso affermativo si effettua la read, sempre se è possibile effettuarla, ossia se non viene
             * chiusa la segreteria.
             */
            char buffer[256];

            if (read(client_socket, buffer, sizeof(buffer)) > 0) {
                printf("Received request: %s\n", buffer);

                    /*
                * Example of requests:
                * 0,Reti di calcolatori,2024/03/01 --> add new exam
                * 1,Reti di calcolatori,0124002583,2024/03/01 --> book exam
                * 2,Reti di calcolatori --> get dates by exam
                */
                char *token = strtok(buffer, ",");
                int operation = atoi(token);

                if(operation == 0) {
                    //add new exam
                    char *exam = strtok(NULL, ",");
                    char *date = strtok(NULL, ",");

                    //check request format
                    if (exam == NULL || date == NULL) {
                        printf("Missing argument: Usage: 0,<exam_name>,<exam_date>\n");
                        write(client_socket, "Missing argument: Usage: 0,<exam_name>,<exam_date>\n", sizeof("Missing argument: Usage: 0,<exam_name>,<exam_date>\n"));
                    } else {
                        int result = add_exam(exam, date);

                        if(result < 0) {
                            //error adding exam (file error or exam already present)
                            write(client_socket, "Error adding exam\n", sizeof("Error adding exam\n"));
                        } else {
                            write(client_socket, "Exam added\n", sizeof("Exam added\n"));
                        }
                    }
                    

                } else if (operation == 1) {
                    //book exam
                    char* exam = strtok(NULL, ",");
                    char* student_id = strtok(NULL, ",");
                    char* date = strtok(NULL, ",");

                    //check request format
                    if (exam == NULL || student_id == NULL || date == NULL) {
                        write(client_socket, "Missing argument: Usage: 1,<exam_name>,<student_id>,<exam_date>\n", sizeof("Missing argument: Usage: 1,<exam_name>,<student_id>,<exam_date>\n"));
                    } else {
                        int result = book_exam(exam, student_id, date);

                        if(result < 0) {
                            //error booking exam
                            write(client_socket, "Error booking exam\n", sizeof("Error booking exam\n"));
                        } else {
                            printf("Exam booked\n");
                            write(client_socket, "Exam booked\n", sizeof("Exam booked\n"));
                        }
                    }
                } else if (operation == 2) {
                    //get dates by exam
                    char *exam = strtok(NULL, ",");

                    //check request format
                    if (exam == NULL) {
                        write(client_socket, "Missing argument: Usage: 2,<exam_name>\n", sizeof("Missing argument: Usage: 2,<exam_name>\n"));
                    } else {
                        char buffer [256];
                        int result = get_exam_dates(exam, buffer);
                        if(result < 0) {
                            //error getting exam dates
                            write(client_socket, "Error getting exam dates\n", sizeof("Error getting exam dates\n"));
                        } else
                
                        if(buffer[0] == '\0') {
                            char msg [256];
                            sprintf(msg, "No dates found for exam %s\n", exam);
                            printf("%s\n", msg);
                            write(client_socket, msg, sizeof(msg));
                        } else {
                            buffer[strlen(buffer)] = '\n';
                            write(client_socket, buffer, strlen(buffer));
                        }
                    }
                    
                } else {
                    //invalid operation
                    write(client_socket, "Invalid operation\n", sizeof("Invalid operation\n"));
                }
            }
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
    Return 0 if exam added, -2 if exam already present
*/
int add_exam(char *exam, char *date) {

    exam[strcspn(exam, "\n")] = '\0';
    date[strcspn(date, "\n")] = '\0';

    //open file in read mode
    FILE *file = fopen("exams.txt", "r");
    if(file == NULL) {
        perror("Error opening file");
        return -1;
    }

    //check if exam is already present
    char line[256];
    while(fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; //remove newline character
        // check if exam is already present. A line is in the format: exam,date
        char *exam_name = strtok(line, ",");
        if(strcmp(exam_name, exam) == 0 && strcmp(strtok(NULL, ","), date) == 0) {
            fclose(file);
            return -2;
        }
    }

    fclose(file);

    //open file in append mode
    file = fopen("exams.txt", "a");
    if(file == NULL) {
        perror("Error opening file");
        return -1;
    }

    //add exam
    fprintf(file, "%s,%s\n", exam, date);
    fclose(file);
    return 0;
}

int book_exam(char *exam, char *student_id, char *date) {
    exam[strcspn(exam, "\n")] = '\0';
    student_id[strcspn(student_id, "\n")] = '\0';
    date[strcspn(date, "\n")] = '\0';

    //open file in read mode
    FILE *file = fopen("bookings.txt", "r");
    if(file == NULL) {
        perror("Error opening file");
        return -1;
    }

    //check if exam is already booked
    char line[256];
    while(fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; //remove newline character
        // check if exam is already booked. A line is in the format: exam,student_id,date
        char *exam_name = strtok(line, ",");
        if(strcmp(exam_name, exam) == 0 && strcmp(strtok(NULL, ","), student_id) == 0 && strcmp(strtok(NULL, ","), date) == 0) {
            fclose(file);
            return -2;
        }
    }

    fclose(file);

    //open file in append mode
    file = fopen("bookings.txt", "a");
    if(file == NULL) {
        perror("Error opening file");
        return -1;
    }

    //book exam
    fprintf(file, "%s,%s,%s\n", exam, student_id, date);
    fclose(file);
    return 0;
}

int get_exam_dates(char* exam, char* dates) {
    exam[strcspn(exam, "\n")] = '\0';

     while(isspace((unsigned char)exam[strlen(exam) - 1])) {
        exam[strlen(exam) - 1] = '\0';
    }

    //open file in read mode
    FILE *file = fopen("exams.txt", "r");
    if(file == NULL) {
        perror("Error opening file");
        return -1;
    }

    char line[256];

    while(fgets(line, sizeof(line), file)) {
        // check if exam is present. A line is in the format: exam,date
        line[strcspn(line, "\n")] = '\0';
        char *exam_name = strtok(line, ",");
        if (exam_name == NULL) {
            continue;
        }

        //check if exam is present
        if(strcmp(exam_name, exam) == 0) {
            //get exam date
            char *date = strtok(NULL, ",");

            //remove newline and carriage return characters
            date[strcspn(date, "\n")] = '\0';
            date[strcspn(date, "\r")] = '\0';

            //add date to dates string
            if(date != NULL) {
                char temp[256];
                if(strlen(dates) == 0) {
                    sprintf(temp, "%s", date);
                } else {
                    sprintf(temp, "%s,\n%s", dates, date);
                
                }
                strcpy(dates, temp);
            }
        }
    }

    fclose(file);
    return 0;
}