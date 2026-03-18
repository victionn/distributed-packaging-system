#include "../../include/tree/merkletree.h"
#include "../../include/chk/pkgchk.h"
#include "../../include/crypt/sha256.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define SHA256_BFLEN 1024

char** hashChunk(struct bpkg_obj* bpkg) {
    //Allocate memory to store the hashed chunks
    char** hashes = malloc(bpkg -> nchunks * sizeof(char*));
    // Open the file                                                                                                                                                                                                                                                                                                                                                     0
    int f = open(bpkg -> filename, O_RDONLY);
    if (f == -1) {
        printf("Failed to open file\n");
        return NULL;
    }
    

    struct stat sb;
    // Retrieve information on the file
    if (fstat(f, &sb) == -1) {
        printf("Failed to retrieve information\n");
        close(f);
        return NULL;
    }
    
    // Map file into memory held by "contents"
    char *contents = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, f, 0); 
    if (contents == MAP_FAILED) {
        printf("Failed to map file into memory\n");
        close(f);
        return NULL;
    }
    for (int i = 0; i < bpkg -> nchunks; i++) {
        // Initialize the sha256 data
        struct sha256_compute_data data;
        sha256_compute_data_init(&data);
        
        // Get offset and size for the chunks
        struct chunkinfo* c = &bpkg -> chunks[i];
        int offset = c -> offset;
        int datasize = c -> size;
        
        // Update the hash with the chunk data, ensuring all data are read and hashed
        while (datasize > 0) {
            //Use a ternary operator to set read size, ensuring read size does not exceed SHA256_BFLEN
            int readsize = datasize < SHA256_BFLEN ? datasize : SHA256_BFLEN;
            sha256_update(&data, contents + offset, readsize);
            offset += readsize;
            datasize -= readsize;
        
        }
        // Finalize the hash computation
        char final_hash[65];
        uint8_t hash[SHA256_INT_SZ];
        sha256_finalize(&data, hash);
        sha256_output_hex(&data, final_hash);
        //Allocate memory and duplicate final_hash to store the hash
        final_hash[64] = '\0';
        hashes[i] = strdup(final_hash);     
    }   
    
    // Close necessary files/objects
    munmap(contents, sb.st_size);
    close(f);
    
    return hashes;
}

//Create the node, allocating memory to it
struct tree_node* createNode() {
    struct tree_node* node = malloc(sizeof(struct tree_node));
    node -> parent = NULL;
    node -> left = NULL;
    node -> right = NULL;
    node -> is_leaf = 0;
    if (!node) {
        perror("Failed to allocate memory for tree node");
        exit(EXIT_FAILURE);
    }
    return node;

}
//Computes the hash of hash(left.hash + right.hash), given the right node
void computeParentHash(struct tree_node* currentnode) {
    char combinedhash[2*(SHA256_HEXLEN + 1)];
    //Format the combination of both hashes together using snprintf
    snprintf(combinedhash, sizeof(combinedhash), "%s%s", currentnode -> parent -> left -> computed_hash, currentnode -> parent -> right -> computed_hash);
    //Compute the hash of the concatenated string
    struct sha256_compute_data data;
    sha256_compute_data_init(&data);
    sha256_update(&data, (uint8_t*)combinedhash, strlen(combinedhash));
    uint8_t hash[SHA256_INT_SZ];
    sha256_finalize(&data, hash);
    sha256_output_hex(&data, currentnode -> parent -> computed_hash);
    currentnode -> parent -> computed_hash[SHA256_HEXLEN] = '\0';
}
struct merkle_tree* initialiseTree() {
    struct merkle_tree* tree = malloc(sizeof(struct merkle_tree));
    tree -> root = createNode();
    tree -> root -> parent = NULL;
    return tree;
}
void recursiveTree(int depth, int currentdepth, struct tree_node* currentnode, int position, char** hash) {
    //If leaf node hasn't been reached
    if (currentdepth != depth) {

        //Create the left and right child, setting itself as the parent
        currentnode -> left = createNode();
        currentnode -> left -> parent = currentnode;
        currentnode -> right = createNode();
        currentnode -> right -> parent = currentnode;

        //Recursively call this again, this time adding a depth
        recursiveTree(depth, currentdepth + 1, currentnode -> left, position * 2, hash);
        recursiveTree(depth, currentdepth + 1, currentnode -> right, position * 2 + 1, hash);

        //If the node is not the root
        if (currentnode -> parent != NULL) {
            //If the node is the right child of its parent
            if(currentnode -> parent -> right == currentnode) {
                computeParentHash(currentnode);
            }
        }
    }
    //If leaf node has been reached
    else{
        currentnode -> is_leaf = 1;
        currentnode -> computed_hash[64] = '\0';
        currentnode -> position = position;
        strncpy(currentnode -> computed_hash, hash[position], 64); 

        //If the node is the right child of its parent
        if(currentnode -> parent -> right == currentnode) {
            computeParentHash(currentnode);
            //Implement here
        }
    }
}
//Operation queuePop and queuePush to support the level order traversal of the merkle tree, to add the expected hash
struct tree_node* queuePop(struct nodeQueue* queue) {
    if (queue -> first == NULL) {
        return NULL;

    }
    struct queueObject* previousfirst = queue -> first;
    struct tree_node* nodetoreturn = previousfirst -> node;
    //If this is the only element
    if (queue -> first -> next != NULL) {
        struct queueObject* toFree = queue -> first;
        queue -> first = queue -> first -> next;
        free(toFree);
    }
    else {
        struct queueObject* toFree = queue -> first;
        queue -> first = NULL;
        free(toFree);
    }
    return nodetoreturn;
}

void queuePush(struct nodeQueue* queue, struct tree_node* node) {
    struct queueObject* object = malloc(sizeof(struct queueObject));
    object -> node = node;
    object -> next = NULL;
    //If the queue is empty
    if(queue -> first == NULL) {
        queue -> first = object;
        return;
    }
    
    struct queueObject* currentObject = queue -> first;
    //Traverse to the end of the queue
    while (currentObject -> next != NULL) {
        currentObject = currentObject -> next;
    }
    currentObject -> next = object;
    return;
}

int queueEmpty(struct nodeQueue* queue) {
    if (queue -> first == NULL) {
        return 1;
    }
    return 0;
}

void freeQueue(struct nodeQueue* queue) {
    struct queueObject* tofree = queue -> first;
    while(tofree != NULL) {
        struct queueObject* next = tofree -> next;
        free(tofree);
        tofree = next;
    } 

}


void levelOrderAddHash(struct merkle_tree* tree, struct bpkg_obj* bpkg) {
    int index = 0;
    int index2 = 0;
    //Initialise the queue
    struct nodeQueue* queue = malloc(sizeof(struct nodeQueue));
    queue -> first = NULL;
    queuePush(queue, tree -> root);


    while(queueEmpty(queue) == 0) {
        struct tree_node* currentnode = queuePop(queue);
        //Copy the expected hash from the bpkg object into the node
        if (index < bpkg -> nhashes) {
            strncpy(currentnode -> expected_hash, bpkg -> hashes[index], 64);
            index ++;
        }
        else {
            strncpy(currentnode -> expected_hash, bpkg -> chunks[index2].hashvalue, 64);
            index2 ++;
        }
        //Add to the queue
        if (currentnode -> left != NULL) {
            queuePush(queue, currentnode -> left);
        }
        if (currentnode -> right != NULL) {
            queuePush(queue, currentnode -> right);
        }
        
    }
    freeQueue(queue);
    free(queue);
    return;
} 

void freeTree(struct merkle_tree* tree) {
    //Use the queue to free the tree
    struct nodeQueue* queue = malloc(sizeof(struct nodeQueue));
    queue -> first = NULL;
    queuePush(queue, tree -> root);
    //Begin iterating through the tree and freeing the nodes in a level order traversal
    while(queueEmpty(queue) == 0) {
        struct tree_node* currentnode = queuePop(queue);
        if (currentnode -> left != NULL) {
            queuePush(queue, currentnode -> left);
        }
        if (currentnode -> right != NULL) {
            queuePush(queue, currentnode -> right);
        }
        free(currentnode);
    }
    free(tree);
    free(queue);
}

void freeChunkHash(char** hash, struct bpkg_obj* obj) {
    for(int i = 0; i < obj -> nchunks; i++) {
        free(hash[i]);
    }
    free(hash);
}