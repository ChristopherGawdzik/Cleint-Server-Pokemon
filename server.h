#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

// Node struct to represent linked list
typedef struct Node{
    char* line;
    struct Node* next;
} Node;

// function headers
void* searchPokemonWithType(void* context);
void promptForFile();
int getNumber(char buffer[30]);

// global variables
FILE* inputFile;