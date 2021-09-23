#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/semaphore.h>
#include "server.h"

Node* head;
FILE* inputFile;
sem_t mutex;

int main() {

    // checks if user inputted file exists or not
    promptForFile();

    sem_init(&mutex, 0, 1);

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddr;
    unsigned int addrSize;
    fd_set readfds, writefds;
    int status, bytesRcv;
    char buffer[30];
    char *response;

    // Create the server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket < 0) {
        printf("*** SERVER ERROR: Could not open socket.\n");
        exit(-1);
    }

    // Setup the server address
    memset(&serverAddress, 0, sizeof(serverAddress)); // zeros the struct
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons((unsigned short) SERVER_PORT);

    // Bind the server socket
    status = bind(serverSocket, (struct sockaddr *)&serverAddress,sizeof(serverAddress));
    if (status < 0) {
        printf("*** SERVER ERROR: Could not bind socket.\n");
        exit(-1);
    }

    // Set up the line-up to handle up to 5 clients in line
    status = listen(serverSocket, 5);
    if (status < 0) {
        printf("*** SERVER ERROR: Could not listen on socket.\n");
        exit(-1);
    }

    // Wait for clients now
    while (1) {
        addrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket,(struct sockaddr *)&clientAddr,&addrSize);
        if (clientSocket < 0) {
            printf("*** SERVER ERROR: Could accept incoming client connection.\n");
            exit(-1);
        }

        printf("SERVER: Received client connection.\n");
        int userChoice = 0;
        // Go into infinite loop to talk to client
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(serverSocket, &readfds);
            FD_ZERO(&writefds);
            FD_SET(serverSocket, &writefds);

            // Get the message from the client
            bytesRcv = recv(clientSocket, buffer, sizeof(buffer), 0);
            buffer[bytesRcv] = 0; // put a 0 at the end so we can display the string
            printf("SERVER: Received client request: %s\n", buffer);

            // get the pokemon type or file name
            char tempType[30];
            strncpy(tempType, buffer+2, strlen(buffer) - 2);

            userChoice = getNumber(buffer);

            // user inputs 1 to search for pokemon with specific type
            if(userChoice == 1){
                char* pokemonType;
                printf("Awaiting Data From Client \n");
                pokemonType = tempType;

                // creating a thread
                pthread_t t1;
                pthread_create(&t1, NULL, searchPokemonWithType, pokemonType);
            }else if(userChoice == 2) {

                char result[10000] = "\n";
                while(head != NULL){
                    strcat(result, head->line);
                    head = head->next;
                }
                response = result;
            }
            // send back entire string when user chooses to save pokemon that were searched
            if(userChoice == 2){
                printf("SERVER: Sending \"%s\" to client\n", response);
                send(clientSocket, response, strlen(response), 0);
            }else{
                // Respond with an "OK" message
                response = "Ok";
                printf("SERVER: Sending \"%s\" to client\n", response);
                send(clientSocket, response, strlen(response), 0);
            }

            if ((strcmp(buffer,"done") == 0) || (strcmp(buffer,"stop") == 0))
                break;
        }

        printf("SERVER: Closing client connection.\n");
        close(clientSocket); // Close this client's socket
        // If the client said to stop, then I'll stop myself
        if (strcmp(buffer,"stop") == 0)
            break;
    }

    /* CLEAN UP Section */
    fclose(inputFile);

    // to free memory
    while(head != NULL){
        free(head->line);
        head = head->next;
    }

    // Closing the server socket
    close(serverSocket);
    printf("SERVER: Shutting down.\n");
}

// function to check if file exists or not
void promptForFile(){
    // taking in csv file name
    char inputFileName[50];
    printf("Enter the name of the file you would like to create  example -> pokemon.csv \n");
    scanf("%s", inputFileName);

    inputFile = fopen(inputFileName, "r");
    if(inputFile){
        printf("File exists\n");
    }
    // to make sure the file is open or if user wants to exit.
    while(!(inputFile)){
        char newString[50];
        printf("Pokemon file is not found. Please enter the name of the file again. or 'e' to exit\n");
        scanf("%s", newString);
        inputFile = fopen(newString, "r");
        // exit - based on user input
        if(strcmp("e", newString) == 0){
            break;
        }
    }
}

void* searchPokemonWithType(void* searchType) {
    char line[512];
    char *token;

    sem_wait(&mutex);
    // we are in critical section - exactly one thread will execute - other threads are waiting
    // reset the file
    fseek(inputFile, 0, SEEK_SET);

    char* pokeType = (char*)searchType;
    char *newLine;
    // populating array with pokemon lines that have the user inputted pokemon type
    while ((fgets(line, 512, inputFile)) != NULL) {
        // + 1 - to make room for null terminating char
        newLine = malloc(sizeof(char) * strlen(line) + 1);
        // strcpy to save the line as strtok has side effects to parameter passed to it.
        strcpy(newLine, line);
        // split by comma three times to get to the pokemon type
        token = strtok(line, ",");
        token = strtok(NULL, ",");
        token = strtok(NULL, ",");
        // if types match then we store the line in our dynamic array
        if (strcmp(token, pokeType) == 0) {
            Node* newNode = (Node *) malloc(sizeof(Node));
            newNode->next = head;
            head = newNode;
            newNode->line = newLine;
        }
    }
    // unlock mutex
    sem_post(&mutex);

    return NULL;
}

// returns only number from user's inputted string
int getNumber(char buffer[30]){
    int result;
    // getting user's choice/number
    char tempNumber[30];
    strcpy(tempNumber, buffer);
    strtok(tempNumber, ",");
    result = atoi(tempNumber);

    return result;
}