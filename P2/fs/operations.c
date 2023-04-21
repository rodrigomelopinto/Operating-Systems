#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void full_unlock(int *array){
	int i=0;
	while(array[i]!=-1){
		unlock(array[i]);
		i++;
	}
}

int search_vector(int inumber, int *array){
	int i=0;
	while(array[i]!=-1){
		if(array[i]==inumber){
			return SUCCESS;
		}
		i++;
	}
	return FAIL;
}

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {
	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}
	
	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}
	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;
}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
			return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	int array_locks[INODE_TABLE_SIZE],i;

	/*vector that in first position has an iterator*/
	int iterator[1];
	iterator[0] = 0;

	/* use for copy */
	type pType;
	union Data pdata;
	for(i=0;i<INODE_TABLE_SIZE;i++){
		array_locks[i]=-1;
	}
	strcpy(name_copy, name);
	
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	
	parent_inumber = aux_lookup(parent_name,array_locks, iterator);
	
	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		full_unlock(array_locks);
		return FAIL;
	}
	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		full_unlock(array_locks);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		full_unlock(array_locks);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	

	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		full_unlock(array_locks);
		return FAIL;
	}
	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		full_unlock(array_locks);
		return FAIL;
	}

	full_unlock(array_locks);
	
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	int array_locks[INODE_TABLE_SIZE],i;

	/*vector that in first position has an iterator*/
	int iterator[1];
	iterator[0] = 0;

	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	for(i=0;i<INODE_TABLE_SIZE;i++){
		array_locks[i]=-1;
	}

	strcpy(name_copy, name);
	
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	
	parent_inumber = aux_lookup(parent_name,array_locks, iterator);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		full_unlock(array_locks);
		return FAIL;
	}
	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		full_unlock(array_locks);
		return FAIL;
	}
	
	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);
	

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		full_unlock(array_locks);
		return FAIL;
	}

	lock(child_inumber);
	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		full_unlock(array_locks);
		unlock(child_inumber);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		full_unlock(array_locks);
		unlock(child_inumber);
		return FAIL;
	}
	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		full_unlock(array_locks);
		unlock(child_inumber);
		return FAIL;
	}

	full_unlock(array_locks);
	unlock(child_inumber);

	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	int array_locks[INODE_TABLE_SIZE],i,a;
	char *save_ptr;
	
	for(a=0;a<INODE_TABLE_SIZE;a++){
		array_locks[a]=-1;
	}
	i=0;
	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	lockrd(current_inumber);
	array_locks[i]=current_inumber;
	i++;
	/* get root inode data */
	inode_get(current_inumber, &nType, &data);
	
	char *path = strtok_r(full_path, delim,&save_ptr);
	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &save_ptr);
		lockrd(current_inumber);
		array_locks[i]=current_inumber;
		i++;
		inode_get(current_inumber, &nType, &data);
	}
	full_unlock(array_locks);
	return current_inumber;
}

/*
 * Lookup for a given path without unlocking in the end.
 * Input:
 *  - name: path of node
 * 	- array_locks: list of all locked inumbers
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int aux_lookup(char *name,int *array_locks, int *iterator) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *save_ptr;
	
	strcpy(full_path, name);
	
	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;
	char *path = strtok_r(full_path, delim, &save_ptr);
	
	if(path==NULL){
		if(search_vector(current_inumber,array_locks)== FAIL){
			lock(current_inumber);
			array_locks[iterator[0]]=current_inumber;
			iterator[0]++;
		}
	}
	else{
		if(search_vector(current_inumber,array_locks)== FAIL){
			lockrd(current_inumber);
			array_locks[iterator[0]]=current_inumber;
			iterator[0]++;
		}
	}
	
	/* get root inode data */
	inode_get(current_inumber, &nType, &data);
	
	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &save_ptr);
		if(path == NULL){
			if(search_vector(current_inumber,array_locks)== FAIL){
				lock(current_inumber);
				array_locks[iterator[0]]=current_inumber;
				iterator[0]++;
			}
		}
		else{
			if(search_vector(current_inumber,array_locks)== FAIL){
				lockrd(current_inumber);
				array_locks[iterator[0]]=current_inumber;
				iterator[0]++;
			}
		}
		inode_get(current_inumber, &nType, &data);
		
	}
	return current_inumber;
}

/*
 * Moves a node given its path and the new one.
 * Input:
 *  - name: path of node
 *  - name2: new path
 * Returns: SUCCESS or FAIL
 */
int move(char* name, char* name2){
	int child_inumber1, child_inumber2,parent_inumber1,parent_inumber2;
	char *child_name1,*parent_name2, *child_name2, *parent_name1; 
	char name_copy[MAX_FILE_NAME];
	char name_copy2[MAX_FILE_NAME];
	int array_locks[INODE_TABLE_SIZE],i;

	/*vector that in first position has an iterator*/
	int iterator[1];
	iterator[0] = 0;

	/*Prevents moving into the same directory*/
	for(i=0;i<MAX_FILE_NAME;i++){
		if(name[i]!=name2[i]){
			break;
		}
	}

	if(strlen(name)==i){
		printf("failed to move %s, canńot move into the same directory\n",name);
		return FAIL;
	}

	for(i=0;i<INODE_TABLE_SIZE;i++){
		array_locks[i]=-1;
	}
	strcpy(name_copy, name);
	strcpy(name_copy2, name2);

	split_parent_child_from_path(name_copy, &parent_name1, &child_name1);
	split_parent_child_from_path(name_copy2, &parent_name2, &child_name2);

	/*prevents deadlocks*/
	if(strcmp(parent_name2,parent_name1)<0){
		/*gets all the inumbers for the operation*/
		parent_inumber2 = aux_lookup(parent_name2,array_locks, iterator);
		child_inumber2 = aux_lookup(name2,array_locks, iterator);
		parent_inumber1 = aux_lookup(parent_name1,array_locks, iterator);
		child_inumber1 = aux_lookup(name,array_locks, iterator);
	}
	else{
		/*gets all the inumbers for the operation*/
		parent_inumber1 = aux_lookup(parent_name1,array_locks, iterator);
		child_inumber1 = aux_lookup(name,array_locks, iterator);
		parent_inumber2 = aux_lookup(parent_name2,array_locks, iterator);
		child_inumber2 = aux_lookup(name2,array_locks, iterator);
	}
	
	
	
	if(child_inumber2 != FAIL){
		printf("failed to move %s, already exists %s\n",name_copy,name_copy2);
		full_unlock(array_locks);
		return FAIL;
	}
	
	
	if (child_inumber1 == FAIL) {
		printf("failed to move %s, does not exist %s\n",
		        name, name);
		full_unlock(array_locks);
		return FAIL;
	}

	if (parent_inumber2 == FAIL) {
		printf("failed to move %s, does not exist %s\n",
				name, parent_name2);
		full_unlock(array_locks);
		return FAIL;
	}
	
	/*move the inumber to the new location*/
	dir_add_entry(parent_inumber2, child_inumber1, child_name2);

	dir_reset_entry(parent_inumber1,child_inumber1);

	full_unlock(array_locks);
	
	return SUCCESS;
}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}
