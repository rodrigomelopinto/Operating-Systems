#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <strings.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>

#define TRUE 1
#define MAX_INPUT_SIZE 100

int numthreads;

socklen_t addrlen;

int sockfd;

/*---------------------------------------------------------
| Auxiliary function responsible for prior cleaning
| and assigning the name to the server socket
---------------------------------------------------------*/

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

/*---------------------------------------------------------
| Function responsible for removing the commands from the socket
| to slave tasks perform them
---------------------------------------------------------*/

char* removeCommand(struct sockaddr_un *client_addr, int *c) {
    char in_buffer[MAX_INPUT_SIZE];
    addrlen=sizeof(struct sockaddr_un);
    
    c[0] = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,
        (struct sockaddr *)&client_addr[0], &addrlen);

    if(c[0] < 0){
        printf("Erro a ler do socket\n");
        return NULL;
    }
    in_buffer[c[0]]='\0';
    char *command = strdup(in_buffer);
    return command;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void *applyCommands(void *arguments){
    int c[1];
    while (TRUE){
        c[0] = 0;
        struct sockaddr_un client_addr[1];
        char *command = removeCommand(client_addr,c);
        if (command == NULL){
            continue;
        }

        char token;
        char type[MAX_INPUT_SIZE];
        char name[MAX_INPUT_SIZE];

        int output;

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
                        output = create(name, T_FILE);
                        printf("Create file: %s\n", name);
                        //send the answer to the respective client
                        sendto(sockfd, &output, sizeof(output)+1, 0, (struct sockaddr *)&client_addr[0], addrlen);
                        break;
                    case 'd':
                        output = create(name, T_DIRECTORY);
                        printf("Create directory: %s\n", name);
                        //send the answer to the respective client
                        sendto(sockfd, &output, sizeof(output)+1, 0, (struct sockaddr *)&client_addr[0], addrlen);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name);
                output = searchResult;
                //send the answer to the respective client
                sendto(sockfd, &output, sizeof(output)+1, 0, (struct sockaddr *)&client_addr[0], addrlen);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                output = delete(name);
                printf("Delete: %s\n", name);
                //send the answer to the respective client
                sendto(sockfd, &output, sizeof(output)+1, 0, (struct sockaddr *)&client_addr[0], addrlen);
                break;
            case 'm':
                output = move(name,type);
                printf("Move: %s to %s \n", name,type);
                //send the answer to the respective client
                sendto(sockfd, &output, sizeof(output)+1, 0, (struct sockaddr *)&client_addr[0], addrlen);
                break;
            case 'p':
                output = print_tecnicofs(name);
                printf("Print: %s\n",name);
                //send the answer to the respective client
                sendto(sockfd, &output, sizeof(output)+1, 0, (struct sockaddr *)&client_addr[0], addrlen);
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

/*---------------------------------------------------------
| The initial process is responsible for the initialization
| of the server socket and creation of slave threads
| that will execute the commands of each client
---------------------------------------------------------*/
int main(int argc, char* argv[]) {
    int i;
    char *namesocket;
    
    numthreads = atoi(argv[1]);
    namesocket = (char*)malloc(sizeof(char)*(strlen(argv[2])+1));
    strcpy(namesocket, argv[2]);

    
    pthread_t tid[numthreads];

    struct sockaddr_un server_addr;
    char *path;

    if (argc < 2)
        exit(EXIT_FAILURE);

    //inicialize the server socket
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    path = namesocket;

    unlink(path);

    addrlen = setSockAddrUn (namesocket, &server_addr);
    //give a name to the server socket
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
    
    if (chmod(namesocket, 00222) == -1) {
        perror("server:: can't change permissions of socket");
        exit(EXIT_FAILURE);
    } 

    init_fs();
    
    for(i=0; i<numthreads; i++){
        if(pthread_create(&tid[i], 0, &applyCommands, NULL)!=0)
            exit(EXIT_FAILURE);
    }

    for(i=0; i<numthreads; i++){
        if(pthread_join(tid[i], NULL)!=0)
            exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
