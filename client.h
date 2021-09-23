#include <sys/semaphore.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

// Node struct to represent linked list
typedef struct Node{
    char* line;
    struct Node* next;
} Node;
int amountOfQueries;
sem_t mutex;

// function header
void* savePokemonToFile(void* fileName);
void populateNode(char* buffer);