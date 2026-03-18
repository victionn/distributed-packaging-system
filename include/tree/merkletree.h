#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stddef.h>
#include "../../include/chk/pkgchk.h"
#define SHA256_HEXLEN (64)

//Node for the merkle tree
struct tree_node {
    void* value;
    struct tree_node* left;
    struct tree_node* right;
    struct tree_node* parent;
    int position;
    int is_leaf;
    char expected_hash[SHA256_HEXLEN + 1]; 
    char computed_hash[SHA256_HEXLEN + 1];
};

//Merkle tree object holding the root
struct merkle_tree {
    struct tree_node* root;
};

//Queue object used for level order traversal of the merkle tree
struct queueObject {
    struct tree_node* node;
    struct queueObject* next;
};

//Queue to store the head of the queue linked list
struct nodeQueue {
    struct queueObject* first;
};
//Allocates memory for the tree and set its root
struct merkle_tree* initialiseTree();
//Recursively builds the tree and get the computed hash values for each ndoe
void recursiveTree(int depth, int currentdepth, struct tree_node* currentnode, int position, char** hash);
//Gets the hashed chunk values
char** hashChunk(struct bpkg_obj* bpkg);
//Adds the expected hash values to the tree
void levelOrderAddHash(struct merkle_tree* tree, struct bpkg_obj* bpkg);
//Pushes a tree node to the queue
void queuePush(struct nodeQueue* queue, struct tree_node* node);
//Pops a tree node from the queue
struct tree_node* queuePop(struct nodeQueue* queue);
//Checks if the queue is empty
int queueEmpty(struct nodeQueue* queue);
//Free the tree
void freeTree(struct merkle_tree* tree);
//Free the hashes retrieved from the data file
void freeChunkHash(char** hash, struct bpkg_obj* obj);
#endif
