#ifndef FS_H
#define FS_H
#include "state.h"
#include <pthread.h>

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
int aux_lookup(char *name,int *array, int *iterator);
int move(char* name,char* name2);
void print_tecnicofs_tree(FILE *fp);
int print_tecnicofs(char *outputfile);

#endif /* FS_H */
