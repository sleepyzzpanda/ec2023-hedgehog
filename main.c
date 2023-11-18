#define _GNU_SOURCE 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include "list.h"

#define MAXLINE 1040 
#define MSG_LENGTH 1024
#define SERVER_IP "127.0.0.1"

List* inputLst; // global lists for input and output messages
List* outputLst;
List* alertLst;
bool isRunning = true;
pthread_mutex_t inputMutex, outputMutex = PTHREAD_MUTEX_INITIALIZER;

void* keyboardInput(void* arg);
void* printToScreen(void* arg);
void* server_program(void* arg);
void* client_program(void* arg);

void menu_display_user();


// [my port number] [remote machine name] [remote port number]
int main(int argc, char*argv[]){

    (void)argc; // unused
   
    inputLst = List_create();
    outputLst = List_create();
    char* port_send = argv[1];
    char* port_receive = argv[3];
    char* hostname = argv[2];
    pthread_t keyboard_in_thread, receive_thread, print_thread, send_thread; // defining the thread ID's
    int r1, r2, r3, r4;

    // creating the threads
    printf("initiallizing communication with [%s] over port %s\n", hostname, port_send);
    r1 = pthread_create(&keyboard_in_thread, NULL, keyboardInput, NULL); // creating the keyboard thread
    if(r1) {
        printf("Error: pthread_create() failed\n");
        return -1;
    }
    
    r2 = pthread_create(&print_thread, NULL, printToScreen, (void*)hostname); // creating the print thread
    if(r2) {
        printf("Error: pthread_create() failed\n");
        return -1;
    }
    // server thread
    r3 = pthread_create(&receive_thread, NULL, server_program, (void*)port_receive);
    if(r3) {
        printf("Error: pthread_create() failed\n");
        return -1;
    }
    // client thread
    r4 = pthread_create(&send_thread, NULL, client_program, (void*)port_send);
    if(r4) {
        printf("Error: pthread_create() failed\n");
        return -1;
    }
    int s1, s2;
    while(1){
        s1 = pthread_tryjoin_np(keyboard_in_thread, NULL);
        s2 = pthread_tryjoin_np(receive_thread, NULL);

        if(s1 == 0 || s2 == 0){
            break;
        }
    }
    
    pthread_cancel(print_thread);
    pthread_cancel(send_thread);
    if(s1){
        pthread_cancel(keyboard_in_thread);
    }
    if(s2){
        pthread_cancel(receive_thread);
    }
    List_free(inputLst, free);
    List_free(outputLst, free);


    // exiting the program
    printf("Communication with [%s] over port %s has ended\n", hostname, port_send);
    return 0;    
}

void *keyboardInput(void *arg){
    // this function will be used to get input from the keyboard
    // and send it to the sendUDP thread
    char* inputMessage;//this might have to be global in order to free it later correctly
    while(1){
        if(isRunning == false) break;
        inputMessage = malloc(MSG_LENGTH * sizeof(char)); // Allocate memory
        if(inputMessage == NULL) {
            puts("Error: Malloc Failed");
            exit(1);
        }

        fgets(inputMessage, MSG_LENGTH, stdin);

        // lock before getting input
        pthread_mutex_lock(&inputMutex);

        if(List_append(outputLst, inputMessage)){
            printf("Error: List_append() failed\n");
            free(inputMessage); // might still have a memory leak if the thread shuts down and does not execute this line. Will need to free it somewhere we know will execute
        }else{
            if(inputMessage[0] == '!') {
                isRunning = false;
                // break;
            }
        }
        pthread_mutex_unlock(&inputMutex);
    }
    pthread_exit(arg);
}

void *printToScreen(void *arg){
    // this function will be used to print the received UDP datagrams
    // to the screen
    char* hostname_ptr = (char*)arg;
    while(1){
        // lock before getting output
        if(isRunning == false) break;
        pthread_mutex_lock(&outputMutex);

        void* outputMessage = List_first(inputLst);
        char* charPtr = (char*)outputMessage;
        if(charPtr != NULL){
            printf("\n[%s]: %s", hostname_ptr, charPtr);
            if(charPtr[0] == '!') {
                isRunning = false;
            }
        }
        List_remove(inputLst);
        pthread_mutex_unlock(&outputMutex);
    }

    pthread_exit(NULL);
}

// does the receiving
void *server_program(void *arg){
    int sockfd;
    struct addrinfo hints, *servaddr, *p;
    struct sockaddr_storage cliaddr; // client's address information
    socklen_t client_struct_length = sizeof(cliaddr);
    char* port;
    port = (char*) arg;
    char client_message[MAXLINE];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, port, &hints, &servaddr) != 0){
        printf("server get addr failed\n");
        perror("getaddrinfo");
        return NULL;
    }

    for(p = servaddr; p != NULL; p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("Error while creating socket");
            continue;
        }
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    if(p == NULL) {
        fprintf(stderr, "Failed to bind socket\n");
        return NULL;
    }
    
    client_struct_length = sizeof(cliaddr);
    while (1) {
        ssize_t n = recvfrom(sockfd, client_message, MAXLINE, 0, (struct sockaddr *)&cliaddr, &client_struct_length);
        
        if(n > 0){
            client_message[n] = 0; // null terminate the string
            // lock before getting output
            pthread_mutex_lock(&outputMutex);
            List_append(inputLst, &client_message);
            pthread_mutex_unlock(&outputMutex);
            if(client_message[0] == '!') {
                break;
            }
        } else if(n < 0){
            perror("Error receiving data");
        }
    }
    freeaddrinfo(servaddr);
    close(sockfd);
    pthread_exit(NULL);
}

// does the sending
void *client_program(void *arg){
    int sockfd;
    struct addrinfo hints, *servaddr, *p;
    char* port ;
    port = (char*) arg;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if(getaddrinfo(SERVER_IP, port, &hints, &servaddr) != 0){
        printf("client get addr failed\n");
        perror("getaddrinfo");
        return NULL;
    }

    for(p = servaddr; p != NULL; p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("client: socket");
            continue;
        }
        break;
    }

    if(p == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        return NULL;
    }
    while (1){
        // Receiving the second message from the server
        // lock before getting input
        pthread_mutex_lock(&inputMutex);
        void* inputMessage = List_first(outputLst);
        char* charPtr = (char*)inputMessage;
        
        if(charPtr != NULL){
            ssize_t send_len = sendto(sockfd, inputMessage, strlen(inputMessage), 0, p->ai_addr, p->ai_addrlen);
            // printf("\nserver_message:  %s", inputMessage);
            if (send_len == -1) {
                perror("Error sending second message");
                return NULL;
            }
             free(inputMessage);
        } 
        
        List_remove(outputLst);
        pthread_mutex_unlock(&inputMutex);
        
   }
    
    freeaddrinfo(servaddr);
    close(sockfd);
    pthread_exit(NULL);
}


void menu_display_user(){
    int input;
    printf("Welcome to the main menu!\n");
    printf("Please select an option:\n");
    printf("1. See Current Alerts\n");
    printf("2. Report an Incident\n");
    printf("3. See General Information\n");
    printf("4. Exit\n");
    scanf("%d", &input);

    switch(input){
        case 1:
            // see current alerts
            
            break;
        case 2:
            // report an incident
            break;
        case 3:
            // see general information
            break;
        case 4:
            // exit
            break;
        default:
            printf("Invalid input\n");
            break;
    }

    return;
}