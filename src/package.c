#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>


#include "../include/config/config.h"
#include "../include/peer/peer.h"
#include "../include/tree/merkletree.h"
#include "../include/chk/pkgchk.h"
#include "../include/crypt/sha256.h"
#include "../include/package/package.h"

#define MAX_BUFFER 256

void ensureTrailingSlash(char *str) {
    size_t len = strlen(str);
    str[len] = '/';
    str[len + 1] = '\0';
}

int isComplete(struct bpkg_obj* obj) {
    struct bpkg_query query = bpkg_get_min_completed_hashes(obj);
    //If the root isnt returned, then it is incomplete
    if (query.len != 1) {
        bpkg_query_destroy(&query);
        return 0;
    }
    else {
        bpkg_query_destroy(&query);
        return 1;
    }
    return 1;
}
void add_package(char* path, char* file, struct bpkg_list** list) {
    char filepath[MAX_BUFFER];
    char pathtodata[MAX_BUFFER];
    char dir[MAX_BUFFER];
    strcpy(dir, path);
    //Make sure the directory begins with a /
    ensureTrailingSlash(dir);
    int len = strlen(dir);
    //Remove double // at the end if it exists
    if(dir[len - 1] == '/' && dir[len-2] == '/') {
        dir[len - 1] = '\0';
    }
    //Concantentate the directory and file and store it to filepath
    snprintf(filepath, sizeof(filepath) * 2 + 2, "%s%s",dir, file);
    FILE *fii = fopen(filepath, "r");
    if (!fii) {
        printf("Cannot open file\n");
        return;
    }
    fclose(fii);
    struct bpkg_obj* obj = bpkg_load(filepath);
    snprintf(pathtodata, sizeof(pathtodata) * 2 + 1, "%s%s", dir, obj -> filename);
    strcpy(obj -> filename, pathtodata);
    //If data file does not exist, create it
    FILE *fi = fopen(obj -> filename, "r");
    if (!fi) {
        make_file(obj);
    }
    else {
        fclose(fi);
    }
    struct merkle_tree* tree = initialiseTree();
    obj -> tree = tree;

    struct tree_node* root = tree -> root;
    //Get the depth, and the calculated chunk values, and build the merkle tree with the recursive function
    int depth = log2(obj -> nchunks) + 1;
    char** calculatehash = hashChunk(obj);
    recursiveTree(depth, 1, root, 0, calculatehash); //Used to get the computed hashes of each node
    levelOrderAddHash(tree, obj); //Used to get the expected hashses of each node
    freeChunkHash(calculatehash, obj);

    //Add bpkg node to the managed packages
    struct bpkg_list* bpkg_node = malloc(sizeof(struct bpkg_list));
    bpkg_node -> obj = obj;
    bpkg_node -> next = NULL;
    if (*list == NULL) {
        *list = bpkg_node;
        return;
    }
    //Store the bpkg node to the list
    struct bpkg_list* current = *list;
    while (current -> next != NULL) {
        current = current -> next;
    }
    current -> next = bpkg_node;
}


void print_package(struct bpkg_list* list) {
    int counter = 1;
    //If list is empty
    if (list == NULL) {
        printf("No packages managed\n");
    }
    while (list != NULL) {
        char* completestatus;
        //If min hashes returns the root
        if (isComplete(list -> obj) == 1) {
            completestatus = "COMPLETED";
        } else {
            completestatus = "INCOMPLETE";
        }
        printf("%d. %.32s, %s : %s\n", counter, list -> obj -> identifier, list -> obj -> filename, completestatus);
        list = list -> next;
        counter++;

    }
}

void remove_package(struct bpkg_list** list, const char* identifier) {
    struct bpkg_list* current = *list;
    struct bpkg_list* prev = NULL;
    int length = strlen(identifier);
    //Traverse the list to find the node
    while (current != NULL) {
        if (strncmp(current -> obj -> identifier, identifier, length) == 0) {
            if (prev == NULL) {
                *list = current -> next;
            } else {
                prev -> next = current -> next;
            }
            // Free the memory allocated for the package
            freeTree(current -> obj -> tree);
            bpkg_obj_destroy(current -> obj);
            free(current);
            printf("Package has been removed\n");
            return;
        }
        prev = current;
        current = current -> next;
    }
    //If the loop exits, no node has been found
    printf("Identifier provided does not match managed packages\n");
}

void free_package(struct bpkg_list* list) {
    while (list != NULL) {
        struct bpkg_list* current = list;
        list = list -> next;
        freeTree(current -> obj -> tree);
        bpkg_obj_destroy(current -> obj);
        free(current);
    }
}