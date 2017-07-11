#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASHSIZE 128 // 128 ascii characters

static int uniqueCharacterCount;
static int binaryTableCount;
static struct dictionary* hashtable[HASHSIZE];

int readFile(char*);
int addToHashTable(int);
struct dictionary* lookup(int);
unsigned char* lookupNodeCode(int);
unsigned hash(int);
int matchBinaryString(char*, int);
static struct simpleNode* binaryTableIn[HASHSIZE];
static struct node* binaryTableOut[HASHSIZE];
char* checkFileExtension(char*);

struct dictionary // used by hash table to count character frequencies
{
    int character;
    int count;
    struct dictionary *next;
};

struct node
{
    int character;
    int frequency;
    struct node* leftChild;
    struct node* rightChild;
    unsigned char* binary;
};

typedef struct priorityQueue{
    struct node* heap;
    int size;
} priorityQueue;

struct simpleNode // used to fill out binaryTableIn[] when extracting .huff file
{
    int character;
    unsigned char* binary;
};



// =============== Priority Queue Functions ===================
void insert(struct node aNode, struct node* heap, int size) {
    int index = size;
    struct node tmp;
    heap[index] = aNode;
    while (heap[index].frequency < heap[(index-1)/2].frequency && index >= 1) {
        tmp = heap[index];
        heap[index] = heap[(index-1)/2];
        heap[(index-1)/2] = tmp;
        index /= 2;
    }
}
void shiftDown(struct node* heap, int size, int idx) {
    int childID = (idx+1)*2-1; // set to left child index
    struct node tmp;
    for (;;) {
        if (childID > size) {
            break;   // has no child
        }
        //if rightChild is smaller than leftChild
        if (heap[childID].frequency > heap[childID+1].frequency && childID+1 <= size) {
            ++childID; // leftChild becomes rightChild
        }
        if (heap[childID].frequency < heap[idx].frequency) {
            tmp = heap[childID];
            heap[childID] = heap[idx];
            heap[idx] = tmp;
            idx = childID;
        } else {
            break;
        }
    }
}
void enqueue(struct node node, priorityQueue *q) {
    insert(node, q->heap, q->size);
    ++q->size;
}
struct node removeMin(struct node* heap, int size) {
    struct node toRemove = heap[0];
    heap[0] = heap[size-1];
    --size;
    shiftDown(heap, size, 0);
    return toRemove;
}
struct node dequeue(priorityQueue *q) {
    struct node rv = removeMin(q->heap, q->size);
    --q->size;
    return rv;
}
void initializeQueue(priorityQueue *q, int n) {
    q->size = 0;
    q->heap = (struct node*)malloc(sizeof(struct node)*(n));
}

// ===========================================
int readFile(char *fileName)
{
    int characterCount = 0;
    FILE *file_in = fopen(fileName, "rb");
    if(!file_in){
        fprintf(stderr, "%s does not exist!\n", fileName);
    }
    int c;
    while ((c = fgetc(file_in)) != EOF)
    {
        if(addToHashTable(c) == 0) characterCount++;
        else return -1;
    }
    fclose(file_in);
    return characterCount;
}

int traverseFree(struct node* currentNode) // recursively traverse BST to free nodes
{
    if(currentNode->character == '\0')
    {
        traverseFree(currentNode->leftChild);
        traverseFree(currentNode->rightChild);
    }
    else free(currentNode->binary); // free binary from nodes that have binary data
    free(currentNode); // free all nodes
    return 0;
}

int traverse(const char* lr, struct node* currentNode, char *b) // recursively traverse BST, assign Huffman codes
{
    strcat(b, lr);
    if(currentNode->leftChild != NULL)
    {
        traverse("0", currentNode->leftChild, b);
        traverse("1", currentNode->rightChild, b);
    }
    else
    {
        currentNode->binary = malloc(sizeof(char*) * strlen(b));
        strcpy((char*) currentNode->binary, (const char*) b);
        binaryTableOut[binaryTableCount++] = currentNode;
    }
    if(strlen(b) > 0)
    {
        b[strlen(b)-1] = '\0';
    }
    return 0;
}

void huffmanCode(priorityQueue* p)
{
    while(p->size > 1) {
        struct node *temp = calloc(1, sizeof(struct node));
        struct node also = dequeue(p); // dequeue min to left child of temp
        temp->leftChild = malloc(sizeof(struct node));
        temp->leftChild->leftChild = also.leftChild;
        temp->leftChild->rightChild = also.rightChild;
        temp->leftChild->frequency = also.frequency;
        temp->leftChild->character = also.character;
        temp->leftChild->binary = also.binary;
        also = dequeue(p); // dequeue min to be right child of temp
        temp->rightChild = malloc(sizeof(struct node));
        temp->rightChild->leftChild = also.leftChild;
        temp->rightChild->rightChild = also.rightChild;
        temp->rightChild->character = also.character;
        temp->rightChild->frequency = also.frequency;
        temp->rightChild->binary = also.binary;
        temp->frequency = temp->leftChild->frequency + temp->rightChild->frequency;
        temp->character = '\0';
        enqueue(*temp, p); // enqueue temp
        free(temp);
    }
}

int addToHashTable(int character)
{
    if(character > 127)
    {
        return 1; // only want to count them once
    }
    struct dictionary* temp;
    if ((temp = lookup(character)) == NULL) { // not found
        unsigned hashval = hash(character);
        temp = (struct dictionary *) malloc(sizeof(*temp));
        temp->count = 1;
        temp->character = character;
        temp->next = hashtable[hashval];
        hashtable[hashval] = temp;
        uniqueCharacterCount++;
    }
    else
    {
        temp->count++;
    }
    return 0;
}

unsigned hash(int s)
{
    // form a hash value for string s
    unsigned hashval;
    hashval = (unsigned) s + 31;
    return hashval % HASHSIZE;
}

struct dictionary *lookup(int s)
{
    // look for s in hashtab
    struct dictionary *np;
    for (np = hashtable[hash(s)]; np != NULL; np = np->next)
    {
        if (s == np->character) return np;  // return np if found
    }
    return np;        // return NULL if not found
}

unsigned char* lookupNodeCode(int c)
{
    int i;
    for(i = 0; i < HASHSIZE; i++)
    {
        if(binaryTableOut[i]->character == c)
        {
            return binaryTableOut[i]->binary;
        }
    }
    return NULL;
}

char* checkFileExtension(char* fileNameToCheck)
{
    int length = (int) strlen(fileNameToCheck);
    char extension[6];
    memcpy(extension, &fileNameToCheck[length-5], 5);
    extension[5] = '\0';
    if(strcmp(extension, ".huff") != 0)
    {
        return NULL;
    }
    else
    {
        char* fileNameNoExtension = calloc(1, sizeof(char)  * (length));
        memcpy(fileNameNoExtension, &fileNameToCheck[0], ((size_t) length - 5));
        return fileNameNoExtension;
    }
}

int matchBinaryString(char* stringToMatch, int count)
{
    int i;
    for(i = 0; i < count; i++)
    {
        if(strcmp((const char*) binaryTableIn[i]->binary, stringToMatch) == 0)
        {
            return binaryTableIn[i]->character;
        }
    }
    return -1;
}

void writeOutputFile(char* fileIn, char* fileOut, int totalCharacters)
{
    FILE* file_in = fopen(fileIn, "rb");
    if(!file_in){
        fprintf(stderr, "%s does not exist!\n", fileIn);
        exit(1);
    }
    int size;
    fseek(file_in, 0, SEEK_END);
    size = (int) ftell(file_in); // get file size
    fseek(file_in, 0, SEEK_SET);
    if(size == 0){
        fprintf(stderr, "You are literally trying to compress a file with zero bytes.\n");
        exit(1);
    }
    unsigned char* fileInBuffer;
    fileInBuffer = malloc(sizeof(fileInBuffer) * size);
    fread(fileInBuffer, sizeof(fileInBuffer), (size_t) size , file_in); // read bytes into buffer
    fclose(file_in);

    FILE* writeBinary = fopen(fileOut, "wb");
    if(!writeBinary)
    {
        fprintf(stderr, "Can't create file\n");
        exit(1);
    }

    int i;
    for(i = 0; i < uniqueCharacterCount; i++) // write character/Huffman-code data to the .huf file
    {
        fwrite((char*) &binaryTableOut[i]->character, 1, 1, writeBinary);
        fwrite(binaryTableOut[i]->binary, strlen((const char*)binaryTableOut[i]->binary), 1, writeBinary);
        fwrite("\0",1,1,writeBinary);
    }
    fwrite("\x07", 1,1,writeBinary); // marks the end of character/Huffman-code data
    fwrite(&uniqueCharacterCount, sizeof(int), 1, writeBinary);
    fwrite(&totalCharacters, sizeof(int), 1, writeBinary);

    int c, k, byte, byteLen;
    byte = 0, byteLen = 0;
    for(i = 0; i < size; i++) // loop through characters from fileIn
    {
        c = fileInBuffer[i];
        unsigned char *stringBinary = lookupNodeCode(c); // get binary string for given character
        int length = (int) strlen((const char *) stringBinary);

        for (k = 0; k < length; k++) // loop through all chars of given binary string
        {                           // create bit representation. When a byte is made, write it to file & repeat
            if (byteLen == 8) {
                fputc(byte, writeBinary);
                byteLen = 0;
                byte = 0;
            }
            char temp = stringBinary[k];
            if (temp - '0' == 0) {
                byte <<= 1;
                byteLen++;
            } else if (temp - '0' == 1) {
                byte <<= 1;
                byte++;
                byteLen++;
            }
        }

    }
    for(i = byteLen; i < 8; i++) // append 0s to leftover bits to make one last byte
    {
        byte <<= 1;
    }
    fputc(byte, writeBinary);
    free(fileInBuffer);
    fclose(writeBinary);
}

void readOutputFile(char* fileNameToRead, char* fileNameOutput)
{
    size_t size;
    unsigned char* buffer;
    FILE* fileToRead;
    fileToRead = fopen(fileNameToRead, "rb");
    fseek(fileToRead, 0, SEEK_END);
    size = (size_t) ftell(fileToRead);
    fseek(fileToRead, 0, SEEK_SET);
    buffer = (unsigned char*) malloc(sizeof(*buffer) * size);
    int uniqueCharacterCount = 0;
    int totalCharacterCount = 0;
    int charactersCounted = 0;
    if (fileToRead == NULL)
    {
        fprintf(stderr, "Error: There was an error reading the file\n");
        exit(1);
    }
    fread(buffer, sizeof(*buffer), size , fileToRead);
    fclose(fileToRead);
    int i;

    char* currentString = (char*) calloc(1, sizeof(char*) * 128);
    int charTemp;
    int binaryTemp;
    int reachedBinary = 0; // false when huffman table data still needs to be read.

    FILE* outputFile;
    outputFile = fopen(fileNameOutput, "wb");

    struct simpleNode *tempSimpleNode;
    tempSimpleNode = (struct simpleNode *) calloc(1, sizeof(struct simpleNode));
    char* tempBinaryString = calloc(1, HASHSIZE);
    char tempBuffer[4];
    int tableIndex = 0;

    for(i = 0; i < size; i++){
        if (reachedBinary == 0)
        {
            tempSimpleNode->character = buffer[i];
            while(buffer[++i] != 0){
                char z = (char) buffer[i];
                char* c = calloc(1, 2* sizeof(char));
                c[0] = z;
                c[1] = '\0';
                strcat(tempBinaryString, (const char*) c);
                free(c);
            }

            tempSimpleNode->binary = malloc(sizeof(unsigned char*) * strlen(tempBinaryString));
            strcpy((char*) tempSimpleNode->binary, (const char*) tempBinaryString);
            strcpy(tempBinaryString, "");
            binaryTableIn[tableIndex++] = tempSimpleNode;
            tempSimpleNode = (struct simpleNode *) calloc(1, sizeof(struct simpleNode));

            charTemp = buffer[i + 1];
            if (charTemp == 7) {
                reachedBinary = 1;
                i+=2;
                uniqueCharacterCount = *((int *) &buffer[i]);
                i+=4;
                totalCharacterCount = *((int *) &buffer[i]);
                i+=3;
            }
        }
        else{ // binary parsing
            int j;
            for( j = 7; j >= 0; j--)
            {
                sprintf(tempBuffer, "%d", (buffer[i] >> j) & 1);
                strcat(currentString, tempBuffer);
                binaryTemp = matchBinaryString(currentString, uniqueCharacterCount);
                if (binaryTemp != -1) {
                    fputc(binaryTemp, outputFile);
                    strcpy(currentString, "");
                    strcpy(tempBuffer, "");
                    charactersCounted++;
                    if(charactersCounted >= totalCharacterCount)
                    {
                        break;
                    }
                }
            }
        }
    }
    i=0;
    while(binaryTableIn[i] != NULL)
    {
        free(binaryTableIn[i]->binary);
        free(binaryTableIn[i]);
        i++;
    }
    free(tempSimpleNode->binary);
    free(tempSimpleNode);
    free(tempBinaryString);
    free(currentString);
    fclose(outputFile);
    free(buffer);
    printf("Extracting complete.\n");
}

// ===========================================
int main(int argc, char** argv) {

    if(argc > 2) {
        char* fileName = argv[2];
        if(argv[1][0] == 'c') {
            char* binaryString = calloc(1, sizeof(char) * HASHSIZE);
            uniqueCharacterCount = 0;
            binaryTableCount = 0;
            // need to accept various files as input
            int totalNumberOfCharacters = readFile(fileName); // read file, create frequency table of characters
            if(totalNumberOfCharacters == -1)
            {
                fprintf(stderr, "Only 7-bit ascii characters are allowed.\n");
                exit(1);
            }
            priorityQueue pq;
            initializeQueue(&pq, uniqueCharacterCount);
            int i, j;
            j = 0;
            for (i = 0; i < HASHSIZE; i++) {
                if (hashtable[i] != NULL) {
                    struct node *temp1;
                    temp1 = (struct node *) malloc(sizeof(*temp1));
                    temp1->character = hashtable[i]->character;
                    temp1->frequency = hashtable[i]->count;
                    temp1->leftChild = NULL;
                    temp1->rightChild = NULL;
                    enqueue(*temp1, &pq);
                    j++;
                    free(temp1);
                    free(hashtable[i]);
                }
            }
            huffmanCode(&pq);
            if(uniqueCharacterCount == 1) {
                pq.heap->binary = malloc(sizeof(char*));
                strcpy((char*)  pq.heap->binary, (const char*) "0");
                binaryTableOut[binaryTableCount++] = pq.heap;
            }
            else traverse("", pq.heap, binaryString);

            int fileNameLength = (int) strlen(fileName);
            char* fileOut = malloc(sizeof(char) * (fileNameLength + 6));
            strcpy(fileOut, fileName);
            strcat(fileOut, ".huff");

            printf("Writing %s\n", fileOut);
            writeOutputFile(fileName, fileOut, totalNumberOfCharacters);
            printf("Writing complete\n");
            free(fileOut);
            free(binaryString);
            traverseFree(&pq.heap[0]);
        }
        else if(argv[1][0] == 'e'){
            char* fileNameOutput = checkFileExtension(argv[2]);
            if (fileNameOutput == NULL)
            {
                fprintf(stderr, "File must be '.huff' to extract.\n");
            }
            else{
                printf("Extracting\n");
                readOutputFile(fileName, fileNameOutput);
            }
            free(fileNameOutput);
        }
    }
    else fprintf(stderr, "type: \n'./huffman c fileNameToCompress.txt'\nor:\n'./huffman e fileNameToExtract.huff\n");
    return 0;
} // end main()

