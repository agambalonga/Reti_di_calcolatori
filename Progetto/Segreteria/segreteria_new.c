#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

#define SECRETARY_PORT 9003
#define SERVER_PORT 9002

#define max(x, y) ({typeof (x) x_ = (x); typeof (y) y_ = (y); \
x_ > y_ ? x_ : y_;})


int connect_client(char* ip_addr);
int init_server();

int main(int argc, char* argv[]) {
    int sockfd, listenfd;
    int i=0;
    int clientfd[4096];
    int input;
    char name[256];
    char date[11];

    // SEGRETERIA CLIENT

    /**
     * Controllo che venga inserito da riga di comando l'indirizzo IP relativo al server al quale ci si vuole connettere.
     */
    if (argc != 2) {
        fprintf(stderr, "Utilizzo: %s <indirizzoIP>\n", argv[0]);
        exit(1);
    }

    if((sockfd = connect_client(argv[1])) < 0) {
        perror("Errore nella connessione al server");
        return -1;
    }

    // SEGRETERIA SERVER
    if((listenfd = init_server()) < 0) {
        perror("Errore nell'inizializzazione del server");
        return -2;
    }

    /**
     * Ci serviamo di 3 "insiemi" di descrittori per strutturare la successiva select.
     * In particolare abbiamo read_set e write_set che mantengono l'insieme dei descrittori, rispettivamente in lettura
     * e scrittura, e master_set, che permette di reinizializzare read_set e write_set ad ogni iterazione.
     */
    fd_set read_set, write_set, master_set;
    int max_fd;

    /**
     * Inizializzo a 0 tutti i bit relativi ai descrittori del master_set, dopodichè aggiungo a questo array
     * di descrittori sia la socket che permette la connessione al server universitario che quella che permette l'ascolto
     * delle richieste di connessione provenienti dagli studenti.
     * La variabile max_fd serve a specificare quante posizioni degli array di descrittori devono essere controllate
     * all'interno della funzione select.
     */
    FD_ZERO(&master_set);

    FD_SET(sockfd, &master_set);
    FD_SET(listenfd, &master_set);

    max_fd = max(max_fd, listenfd);

    while(1) {
        read_set = master_set;
        write_set = master_set;

        while(1) {
            /**
             * La funzione select restituisce il numero di descrittori pronti, a partire dagli "insiemi" di descrittori
             * passati.
             */
            if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0) {
                perror("Errore nell'operazione di select!");
            }

            /**
             * Si controlla se il descrittore listenfd, ossia quello che monitora le nuove richieste di connessioni,
             * sia pronto, il che è vero quando appunto ci sono nuove connessioni in attesa.
             */
            if (FD_ISSET(listenfd, &read_set)) {
                /**
                 * La system call accept permette di accettare una nuova connessione (lato server) in entrata da un client.
                 */
                if ((clientfd[i] = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
                    perror("Errore nell'operazione di accept!");
                } else {
                    printf("Connessione accettata\n");

                    /**
                     * Si aggiunge il descrittore legato alla nuova connessione da uno studente all'interno dell'array di
                     * descrittori master_set e si ricalcola il numero di posizioni da controllare nella select.
                     */
                    FD_SET(clientfd[i], &master_set);
                    max_fd = max(max_fd, clientfd[i]);

                    i++;
                }
            } else {
                break;
            }
        }

        for(int j=0; j<i; j++) {
            /**
             * Controllo se i descrittori delle socket di connessione dei client sono pronte in lettura, poichè
             * significherebbe che lo studente specifico è pronto in scrittura.
             */
            if(FD_ISSET(clientfd[j], &read_set) && clientfd[j] != -1) {
                char buffer[256];
                char buffer_copy[256];
                char response[256];

                memset(buffer, 0, sizeof(buffer));  // Azzeramento del buffer
                memset(buffer_copy, 0, sizeof(buffer_copy));  // Azzeramento di buffer_copy
                memset(response, 0, sizeof(response));  // Azzeramento di response

                if (read(clientfd[j], buffer, sizeof(buffer)) > 0) {
                    printf("Richiesta ricevuta dal client studente: %s\n", buffer);
                    strcpy(buffer_copy, buffer);
                    
                    char *token = strtok(buffer_copy, ",");
                    int operation = atoi(token);

                    if(operation == 1) {
                        if(write(sockfd, buffer, sizeof(buffer)) < 0) {
                                perror("Error sending request to server");
                                continue;
                            }

                            if (read(sockfd, response, sizeof(response)) < 0) {
                                perror("Error receiving response from server");
                            } else {
                                printf("Received response from server: %s\n", response);
                                write(clientfd[j], response, strlen(response));
                            }
                    } else if(operation == 2) {
                        //send request to server
                        if(write(sockfd, buffer, sizeof(buffer)) < 0) {
                            perror("Error sending request to server");
                            continue;
                        } else {
                            if (read(sockfd, response, sizeof(response)) < 0) {
                                perror("Error receiving response from server");
                            } else {
                                printf("Received response from server: %s\n", response);
                                write(clientfd[j], response, strlen(response));
                            }
                        }
                    } else {
                        //invalid operation
                        write(clientfd[i], "Invalid operation\n", sizeof("Invalid operation\n"));
                    }
                } else {
                    perror("Errore nella lettura della richiesta!");
                }
            }
        }

         /**
         * Controllo se la segreteria è pronta in scrittura.
         */
        /**/
        if(FD_ISSET(sockfd, &write_set)) {

            /**
             * All'interno richiedo alla segreteria se vuole continuare a gestire le richieste degli studenti oppure
             * vuole procedere all'inserimento di un nuovo appello, prima di soddisfare le richieste degli studenti
             * in attesa.
             */
            printf("Vuoi gestire le richieste degli studenti o inserire un nuovo appello?\n");
            printf("1 - Gestire le richieste degli studenti\n");
            printf("2 - Inserire un nuovo appello\n");
            printf("Scelta: ");

            scanf("%d", &input);
            printf("\n");

            /**
             * Pulisco il buffer di input.
             */
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            /**
             * Nel caso in cui si voglia aggiungere un nuovo appello, si procede alla comunicazione con il server
             * universitario.
             */
            if (input == 2) {
                printf("Inserisci il nome dell'esame: ");
                fgets(name, sizeof(name), stdin);
                printf("Inserisci la data dell'esame: ");
                fgets(date, sizeof(date), stdin);

                char request[256];
        
                name[strcspn(name, "\n")] = 0;
                date[strcspn(date, "\n")] = 0;
                sprintf(request, "0,%s,%s", name, date);

                if(write(sockfd, request, strlen(request)) < 0) {
                    perror("Errore nell'invio della richiesta al server!");
                } else {
                    char response[256];
                    if (read(sockfd, response, sizeof(response)) < 0) {
                        perror("Errore nella ricezione della risposta dal server!");
                    } else {
                        printf("Risposta ricevuta:  %s\n", response);
                    }
                }
            }
        }
    }
}


/*
    * Funzione che si occupa di connettere il client al server tramite una socket.
    * Restituisce il descrittore della socket in caso di successo
    * -1 in caso di errore nella chiamata a inet_pton,
    * -2 in caso di errore nella chiamata a connect.  
*/
int connect_client( char* ip_addr) {
    int sockfd;

    /**
     * Utilizzo la system call socket, che prende in input tre parametri di tipo intero, per creare una nuova socket
     * da associare al descrittore "sockfd". I tre parametri in input riguardano, in ordine, il dominio
     * degli indirizzi IP (IPv4 in questo caso), il protocollo di trasmissione (in questo caso TCP), mentre l'ultimo
     * parametro, se messo a 0, specifica che si tratta del protocollo standard.
     */
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        exit(1);
    }


    struct sockaddr_in servaddr;
    /**
     * Specifico la struttura dell'indirizzo del server al quale ci si vuole connettere tramite i campi di una struct
     * di tipo sockaddr_in. Vengono utilizzati indirizzi IPv4, ci si connette all'indirizzo IP sul quale si trova
     * il server, il localhost in questo caso, e la porta su cui sta attendendo il server.
     */
    servaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_addr, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Errore inet_pton per %s\n", ip_addr);
       return -1;
    }
    servaddr.sin_port = htons(SERVER_PORT);

    /**
     * La system call connect permette di connettere la socket al server specificato nella struct "servaddr" tramite
     * l'indirizzo IP e la porta memorizzate nella struttura.
     */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Errore nell'operazione di connect!");
        return -2;
    }

    return sockfd;
}

/*
    * Funzione che si occupa di inizializzare il server della segreteria tramite una socket.
    * Restituisce il descrittore della socket in caso di successo
    * -1 in caso di errore nella chiamata a socket,
    * -2 in caso di errore nella chiamata a bind,
    * -3 in caso di errore nella chiamata a listen.
*/
int init_server() {
    int listenfd;
    struct sockaddr_in secaddr;
    /**
     * Utilizzo la system call socket, che prende in input tre parametri di tipo intero, per creare una nuova socket
     * da associare al descrittore "listenfd". I tre parametri in input riguardano, in ordine, il dominio
     * degli indirizzi IP (IPv4 in questo caso), il protocollo di trasmissione (in questo caso TCP), mentre l'ultimo
     * parametro, se messo a 0, specifica che si tratta del protocollo standard.
     */
    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Errore nella creazione della socket!");
        return -1;
    }

    /**
     * Specifico la struttura dell'indirizzo del server tramite i campi di una struct di tipo sockaddr_in.
     * Vengono utilizzati indirizzi IPv4, vengono accettate connessioni da qualsiasi indirizzo e la porta su cui
     * il server risponderà ai client sarà la 1025.
     */
    secaddr.sin_family = AF_INET;
    secaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    secaddr.sin_port = htons(SECRETARY_PORT);

    /**
     * La system call bind permette di assegnare l'indirizzo memorizzato nel campo s_addr della struct sin_addr, che è
     * a sua volta un campo della struct sockaddr_in (secaddr nel nostro caso), al descrittore listenfd.
     */
    if ((bind(listenfd, (struct sockaddr *)&secaddr, sizeof (secaddr))) < 0) {
        perror("Errore nell'operazione di bind!");
        return -2;
    }

    /**
     * Mettiamo il server in ascolto, specificando quante connessioni possono essere in attesa venire accettate
     * tramite il secondo argomento della chiamata.
     */
    if ((listen(listenfd, 5)) < 0) {
        perror("Errore nell'operazione di listen!");
        return -3;
    }

    printf("Server in ascolto sulla porta %d\n", SECRETARY_PORT);

    return listenfd;
}