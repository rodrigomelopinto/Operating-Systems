#ifndef FS_H
#define FS_H
#include "state.h"
#include <pthread.h>

extern pthread_mutex_t mutex;
extern pthread_rwlock_t rwlock;
extern char *synchstrategy;

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
