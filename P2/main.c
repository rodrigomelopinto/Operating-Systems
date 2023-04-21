#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <sys/time.h>
#include <pthread.h>

#define TRUE 1
#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

char *inputfile,*outputfile;
int numthreads;

pthread_mutex_t mutex;
pthread_cond_t podein;
pthread_cond_t podeout;

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int removeQueue = 0 , insertQueue=0;

int insertCommand(char* data) {
    pthread_mutex_lock(&mutex);
    while(numberCommands == MAX_COMMANDS){
        pthread_cond_wait(&podein,&mutex);
    }
    if(insertQueue != MAX_COMMANDS) {
        strcpy(inputCommands[insertQueue], data);
        insertQueue++;
        if(insertQueue==MAX_COMMANDS){
            insertQueue=0;
        }
        numberCommands++;
        pthread_cond_signal(&podeout);
        pthread_mutex_unlock(&mutex);
        return 1;
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

char* removeCommand() {
    char* command;
    pthread_mutex_lock(&mutex);
    while(numberCommands == 0){
        pthread_cond_wait(&podeout,&mutex);
    }

    if(numberCommands > 0){
        numberCommands--;
        command = strdup(inputCommands[removeQueue]);
        removeQueue++;
        if(removeQueue==MAX_COMMANDS){
            removeQueue=0;
        }
        
        pthread_cond_signal(&podein);
        pthread_mutex_unlock(&mutex);
        return command;  
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(){
    char line[MAX_INPUT_SIZE];
    FILE *fp;
    int i;
    fp = fopen(inputfile,"r");
    if(fp==NULL){
        perror("test1.txt");
        exit(1);
    }

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), fp)) {
        char token;
        char name[MAX_INPUT_SIZE];
        char type[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %s", &token, name, type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'm':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    /*condtion that ends the file*/
    for(i=0;i<numthreads;i++){
        insertCommand("EOF");
    }
    fclose(fp);
}

void *applyCommands(){
    while (TRUE){
        
        char* command = removeCommand();
        
        if(strcmp(command,"EOF")==0){
            free(command);
            return NULL;
        }
       
        
        if (command == NULL){
            continue;
        }

        char token;
        char type[MAX_INPUT_SIZE];
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(command, "%c %s %s", &token, name, type);
        
        if (numTokens < 2) {
            free(command);
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
       
        int searchResult;
        switch (token) {
            case 'c':
                switch (type[0]) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            case 'm':
                printf("Move: %s to %s \n", name,type);
                move(name,type);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        free(command);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    FILE *fp;
    int i;
    struct timeval start,end;
    
    inputfile = (char*)malloc(sizeof(char)*(strlen(argv[1])+1));
    outputfile = (char*)malloc(sizeof(char)*(strlen(argv[2])+1));
    numthreads = atoi(argv[3]);
    strcpy(inputfile, argv[1]);
    strcpy(outputfile, argv[2]);

    
    pthread_t tid[numthreads];
    
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&podein,NULL);
    pthread_cond_init(&podeout,NULL);

    
    fp = fopen(outputfile, "w");
    /* init filesystem */
    init_fs();

    /* process input and print tree */

    gettimeofday(&start,NULL);

    for(i=0; i<numthreads; i++){
        if(pthread_create(&tid[i], 0, applyCommands, NULL)!=0)
            exit(EXIT_FAILURE);
    }

    processInput();

    for(i=0; i<numthreads; i++){
        pthread_join(tid[i],NULL);
    }

    gettimeofday(&end,NULL);
    int seconds = (end.tv_sec -start.tv_sec);
    int micros = ((seconds * 1000000)+ end.tv_usec)-(start.tv_usec);
    float result = seconds +(micros*0.000001) ;
    printf("TecnicoFS completed in %0.4f seconds.\n", result);
    print_tecnicofs_tree(fp);
    fclose(fp);
    /* release allocated memory */
    destroy_fs();
    free(inputfile);
    free(outputfile);
    exit(EXIT_SUCCESS);
}
