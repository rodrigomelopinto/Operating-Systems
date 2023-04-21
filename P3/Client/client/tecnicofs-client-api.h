#ifndef API_H
#define API_H

#include "tecnicofs-api-constants.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

int tfsCreate(char *path, char nodeType);
int tfsDelete(char *path);
int tfsLookup(char *path);
int tfsMove(char *from, char *to);
int tfsMount(char * sockPath);
int tfsUnmount();
int tfsprint(char *outputfile);
int dg_cli(int sockfd, struct sockaddr_un serv_addr, int servlen, char * msg);

#endif /* CLIENT_H */
