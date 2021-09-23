#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "client.h"
#include <pthread.h>

#define BYTESIZE 10000
char* outputFileName;
Node* pokemonLines;

int main() {

    int clientSocket;
    struct sockaddr_in serverAddress;
    int status, bytesRcv;
    char instr[80];
    char* buffer = malloc(sizeof(char) * BYTESIZE); // stores user input from keyboard

    // Create the client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket < 0) {
        printf("Unable to establish connection to the PPS!\n");
        exit(-1);
    }

    // Setup address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddress.sin_port = htons((unsigned short) SERVER_PORT);

    // Connect to server
    status = connect(clientSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (status < 0) {
        printf("*** CLIENT ERROR: Could not connect.\n");
        exit(-1);
    }

    // Go into loop to commuincate with server now
    while (1) {

        // Get a command from the user
        printf("CLIENT: Enter command to send to server\n");

        // display menu to user
        printf("Please select an option 1 - 3\n");
        printf("1. Type search\n");
        printf("2. Save results\n");
        printf("3. Exit the program\n");

        // take in input from user based on menu
        scanf("%s", instr);
        int userChoice = atoi(instr);

        switch(userChoice)
        {
            case 1: // case 1 is when doing a type search
                printf("Please input what type of pokemon you want to choose - Ex. Fire, water etc\n");
                char typeSearch[20];
                scanf("%s", typeSearch);
                // combining into one string
                sprintf(buffer, "%d,%s", userChoice, typeSearch);
                amountOfQueries++;
                break;
            case 2: // case 2 is when saving type searched pokemon into a file
                printf("Please input what you would like to call the file Ex: saveHere.csv \n");
                char fileName[20];
                scanf("%s", fileName);

                // user inputs 2 to save previously searched pokemon to csv file
                FILE* outputFile;
                outputFileName = fileName;
                outputFile = fopen(outputFileName, "a+");

                while(!(outputFile)){
                    printf("Output file could not be created. Please enter the name of the file again");
                    scanf("%s", outputFileName);
                    outputFile = fopen(outputFileName, "a+");
                }
                // combining into one string
                sprintf(buffer, "%d,%s", userChoice,outputFileName);
                break;
            case 3: // case 3 is when exiting the program - print how many queries and final file name created
                printf("There were this many queries in total %d\n", amountOfQueries);
                printf("File created %s\n", outputFileName);
                printf("Time To Exit!\n");
                return EXIT_SUCCESS;
            default:
                printf("Invalid input");
        }

        printf("CLIENT: Sending \"%s\" to server.\n", buffer);
        send(clientSocket,buffer, strlen(buffer), 0);

        // Get response from server and print it
        bytesRcv = recv(clientSocket, buffer, BYTESIZE, 0);
        buffer[bytesRcv] = 0; // put a 0 at the end so we can display the string
        printf("CLIENT: Got back response \"%s\" from server.\n", buffer);

        if(userChoice == 2){
            populateNode(buffer);
            pthread_t t2;
            pthread_create(&t2, NULL, savePokemonToFile, outputFileName);
            //savePokemonToFile(outputFileName);
        }

        if ((strcmp(instr,"Done") == 0) || (strcmp(instr,"Stop") == 0))
            break;
    }

    close(clientSocket); // Don't forget to close the socket !
    printf("CLIENT: Shutting down.\n");
}

// function takes in a string and puts line by line into a linked list
void populateNode(char* buffer){
    char* token;
    token = strtok(buffer,"\n");
    char *newLine;

    while (token != NULL) {
        // + 1 - to make room for null terminating char
        newLine = malloc(sizeof(char) * strlen(token) + 1);
        // strcpy to save the line as strtok has side effects to parameter passed to it.
        strcpy(newLine, token);
        // Making sure that linked list is initlized properly and line is set to newLine each time
        Node* newNode = (Node *) malloc(sizeof(Node));
        newNode->next = pokemonLines;
        pokemonLines = newNode;
        newNode->line = newLine;

        token = strtok(NULL, "\n");
    }
}
// write pokemon array to file function
void* savePokemonToFile(void* fileName){
    sem_wait(&mutex);

    // create file
    FILE* file;
    // open file with fileName in append mode so multiple pokemon type can be in the same file
    file = fopen(fileName, "a+");

    Node* current = pokemonLines;

    // printing to file
    while(current != NULL) {
        fprintf(file,"%s\n", current->line);
        current = current->next;
    }
    // close the file we created earlier
    fclose(file);

    sem_post(&mutex);

    return NULL;
}