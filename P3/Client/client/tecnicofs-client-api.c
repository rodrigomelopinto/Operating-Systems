#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

int socketfd;
int server_len;
char clientname[21];
struct sockaddr_un server_address;

int tfsCreate(char *filename, char nodeType) {
  char msg[MAX_INPUT_SIZE] = "c ";
  int res;
  strcat(msg,filename);
  strcat(msg," ");
  strcat(msg,&nodeType);
  res = dg_cli(socketfd, server_address,server_len,msg);
  return res;
}

int tfsDelete(char *path) {
  char msg[MAX_INPUT_SIZE] = "d ";
  int res;
  strcat(msg,path);
  res = dg_cli(socketfd, server_address,server_len,msg);
  return res;
}

int tfsMove(char *from, char *to) {
  char msg[MAX_INPUT_SIZE] = "m ";
  int res;
  strcat(msg,from);
  strcat(msg," ");
  strcat(msg,to);
  res = dg_cli(socketfd, server_address,server_len,msg);
  return res;
}

int tfsLookup(char *path) {
  char msg[MAX_INPUT_SIZE] = "l ";
  int res;
  strcat(msg,path);
  res = dg_cli(socketfd, server_address,server_len,msg);
  return res;
}

/*---------------------------------------------------------
| Auxiliary function responsible for sending and receiving
| information from the server
---------------------------------------------------------*/
int dg_cli(int sockfd, struct sockaddr_un serv_addr, int servlen, char * msg){
  int output;
  
  if (sendto(sockfd, msg, strlen(msg)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 
  
  if (recvfrom(sockfd, &output, sizeof(output), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 
  
  if(output >=0){
    return 0;
  }
  else{
    return -1;
  }
}

/*---------------------------------------------------------
| Function responsible for creating the socket for each client
---------------------------------------------------------*/
int tfsMount(char * sockPath) {
  int sockfd,clilen,servlen;
  char cliname[20] = "/tmp/Client";
  struct sockaddr_un cli_addr,serv_addr;
  int pid = getpid();
  char pidc[21];
  sprintf(pidc,"%d",pid);
  strcat(cliname,pidc);

  if((sockfd = socket(AF_UNIX,SOCK_DGRAM,0))<0){
    printf("erro na criação do socket\n");
    exit(EXIT_FAILURE);
  }

  unlink(cliname);
  bzero((char*) &cli_addr,sizeof(cli_addr));
  cli_addr.sun_family = AF_UNIX;
  strcpy(cli_addr.sun_path,cliname);
  clilen = sizeof(cli_addr.sun_family)+ strlen(cli_addr.sun_path);

  if(bind(sockfd,(struct sockaddr *) &cli_addr,clilen)<0){
    printf("erro na associação de um nome ao socket\n");
    exit(EXIT_FAILURE);
  }
  printf("criou o socket\n");
  bzero((char*) &serv_addr,sizeof(serv_addr));

  serv_addr.sun_family= AF_UNIX;
  strcpy(serv_addr.sun_path,sockPath);
  servlen = sizeof(serv_addr.sun_family) + strlen(serv_addr.sun_path);
  socketfd = sockfd;
  server_address = serv_addr;
  server_len = servlen;
  strcpy(clientname,cliname);

  return 0;
}

/*---------------------------------------------------------
| Function responsible for destroying the socket of the client
---------------------------------------------------------*/
int tfsUnmount() {
  close(socketfd);

  unlink(clientname);
  
  return 0;
}

int tfsprint(char *outputfile){
  char msg[MAX_INPUT_SIZE] = "p ";
  int res;
  strcat(msg,outputfile);
  res = dg_cli(socketfd, server_address,server_len,msg);
  return res;
}