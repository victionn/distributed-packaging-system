#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "../../include/chk/pkgchk.h"
#include "../../include/tree/merkletree.h"
#define SHA256_HEX_LEN (64)

int is_not_hexadecimal(const char *str) {
    // Iterate over each character in the string
    while (*str) {
        if (!isxdigit((unsigned char)*str)) {
            return 1;
        }
        str++;
    }
    return 0; 
}


// PART 1

/**
 * Loads the package for when a valid path is given
 */
struct bpkg_obj* bpkg_load(const char* path) {
    //Allocate memory for the object
    struct bpkg_obj* obj = malloc(sizeof(struct bpkg_obj));

    //Attempt to open the file specified
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Unable to load pkg and tree\n");
        free(obj);
        exit(1);
    }

    int counter = 0;
    char buffer[1036];
    //Reads in the contents of the bpkg file
    while (fgets(buffer, sizeof(buffer), file)) {
        if (counter == 0) {
            sscanf(buffer, "ident: %s\n", obj -> identifier);
            if(is_not_hexadecimal(obj->identifier)) {
                printf("Unable to load pkg and tree\n");
                free(obj);
                exit(1);
            }
        }

        if(counter == 1) {
            sscanf(buffer, "filename: %s\n", obj -> filename);
        }

        if(counter == 2) {
            sscanf(buffer, "size: %d\n", &obj -> size);
        }

        if(counter == 3) {
            sscanf(buffer,"nhashes: %d", &obj -> nhashes);
        }

        if (counter == 4) {
            //Allocate memory to the array of pointers
            obj -> hashes = malloc(obj -> nhashes * sizeof(char*));
            for(int i = 0; i < obj -> nhashes; i ++) {
                //Allocate memory to the pointer holding the hashes
                obj -> hashes[i] = malloc(MAX_HASH + 1);
                fgets(buffer, sizeof(buffer), file);
                sscanf(buffer, "%64s", obj -> hashes[i] );
            }
        }

        if (counter == 5) {
            sscanf(buffer, "nchunks:%d", &obj -> nchunks);
            //If nchunks and nhashes doesnt match up, indicating the merkle tree is not perfect
            if (obj->nchunks != obj->nhashes + 1) {
                printf("Unable to load pkg and tree\n");
                free(obj);
                exit(1);
            }
        }

        if (counter == 6) {
            //Stores the chunk, offset and size into the object
            obj -> chunks = malloc(obj -> nchunks * sizeof(struct chunkinfo));
            for(int i = 0; i < obj -> nchunks; i++) { 
                fgets(buffer, sizeof(buffer), file);
                sscanf(buffer, "%*[, \t]%65[^,], %d, %d", obj -> chunks[i].hashvalue, &obj -> chunks[i].offset, &obj -> chunks[i].size);
            }
        }
        counter = counter + 1;
    }
    //Free memory
    fclose(file);
    return obj;
}

/**
 * Checks to see if the referenced filename in the bpkg file
 * exists or not.
 * @param bpkg, constructed bpkg object
 * @return query_result, a single string should be
 *      printable in hashes with len sized to 1.
 * 		If the file exists, hashes[0] should contain "File Exists"
 *		If the file does not exist, hashes[0] should contain "File Created"
 */
struct bpkg_query bpkg_file_check(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    qry.hashes = malloc(sizeof(char*));
    qry.hashes[0] = malloc(SHA256_HEX_LEN);
    FILE *f = fopen(bpkg -> filename, "r");
    
    //If file does not exist, we create a file with the size specified in the bpkg file
    if (!f) {
        //Create the file with read/write permissions
        make_file(bpkg);
        strcpy(qry.hashes[0], "File created");
    }

    //If file exists, we set the first element in hashes to "File exists", and close the file
    else{
        strcpy(qry.hashes[0], "File Exists");
        fclose(f);
    }
    qry.len = 1;
    return qry;
}
/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    qry.hashes = malloc(sizeof(char*) * (bpkg -> nhashes + bpkg -> nchunks));
    //Iterate through all of the hashes in the object, and strcpy it into the query object
    for(int i = 0; i < bpkg -> nhashes; i++) {
        qry.hashes[i] = malloc(SHA256_HEX_LEN + 1);
        strcpy(qry.hashes[i], bpkg -> hashes[i]);
    } 
    //Iterate through the chunk hashes, and strcpy it into the queue object
    int count = bpkg -> nhashes;
    for(int i = 0; i < bpkg -> nchunks; i++) {
        qry.hashes[count] = malloc(SHA256_HEX_LEN + 1);
        strncpy(qry.hashes[count], bpkg -> chunks[i].hashvalue, 64);
        count ++;
    }
    //Set qry.len to the number of hashes
    qry.len = bpkg -> nhashes + bpkg -> nchunks;
    return qry;
}


struct bpkg_query bpkg_get_completed_chunks(struct bpkg_obj* bpkg) { 
    struct bpkg_query qry = { 0 };
    qry.len = 0;
    qry.hashes = malloc(sizeof(char*) * bpkg -> nchunks);
    struct nodeQueue* queue = malloc(sizeof(struct nodeQueue));
    queue -> first = NULL;
    queuePush(queue, bpkg -> tree -> root);
    //initialise level order traversal to get to the leaf nodes (chunks)
    while (queueEmpty(queue) == 0) {
        struct tree_node* currentnode = queuePop(queue);
        //Add the left and right child to the queue
        if (currentnode -> left != NULL && currentnode -> right != NULL) {
            queuePush(queue, currentnode -> left);
            queuePush(queue, currentnode -> right);
        }

        //If leaf node reached
        if (currentnode -> is_leaf == 1) {
            //If computed hash = expected hash
            if (strncmp(currentnode -> expected_hash, currentnode -> computed_hash, 64) == 0) {
                qry.hashes[qry.len] = malloc(SHA256_HEX_LEN + 1);
                strncpy(qry.hashes[qry.len], currentnode -> expected_hash, 64);
                qry.len += 1;
            } 
        }

    }
    free(queue);
    return qry;
}
/**
 * Gets only the required/min hashes to represent the current completion state
 * Return the smallest set of hashes of completed branches to represent
 * the completion state of the file.
 *
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg) {
    //Initialise
    struct bpkg_query qry = { 0 };
    qry.len = 0;
    qry.hashes = malloc(sizeof(char*) * bpkg -> nchunks);
    struct nodeQueue* queue = malloc(sizeof(struct nodeQueue));
    queue -> first = NULL;
    queuePush(queue, bpkg -> tree -> root);
    struct tree_node* currentnode;

    //Begin level order traversal
    while (queueEmpty(queue) == 0) {
        currentnode = queuePop(queue);
        //If the computed hash matches with the expected hash
        if (strncmp(currentnode -> computed_hash, currentnode -> expected_hash, 64) == 0) {
            qry.hashes[qry.len] = malloc(SHA256_HEX_LEN + 1);
            strncpy(qry.hashes[qry.len], currentnode -> expected_hash, 64);
            qry.len += 1;
        }
        else {
            //If its not a leaf node, add its left and right children
            if (currentnode -> is_leaf == 0) {
                queuePush(queue, currentnode -> left);
                queuePush(queue, currentnode -> right);
            }
            
        }
    }
    free(queue);
    return qry;
}

/**
 * Retrieves all chunk hashes given a certain an ancestor hash (or itself)
 * Example: If the root hash was given, all chunk hashes will be outputted
 * 	If the root's left child hash was given, all chunks corresponding to
 * 	the first half of the file will be outputted
 * 	If the root's right child hash was given, all chunks corresponding to
 * 	the second half of the file will be outputted
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg, char* hash) {
struct bpkg_query qry = { 0 };
    qry.len = 0;
    qry.hashes = malloc(sizeof(char*) * bpkg -> nchunks);
    struct nodeQueue* queue = malloc(sizeof(struct nodeQueue));
    int foundHash = 0;
    queue -> first = NULL;
    queuePush(queue, bpkg -> tree -> root);
    struct tree_node* currentnode;
    while (queueEmpty(queue) == 0) {
        currentnode = queuePop(queue);
        //If the node with the matching hash in the provided parameter is found
        if (strncmp(currentnode -> expected_hash, hash, 64) == 0) {
        foundHash = 1;
        break;
        }
        //If the leaf node hasn't been reached
        if(currentnode -> is_leaf == 0) {
            queuePush(queue, currentnode -> left);
            queuePush(queue, currentnode -> right);
        }
    }
    //If the matching node hasnt been found, return
    if (foundHash == 0) {
        return qry;
    }
    //Free up all objects in queue
    while (queueEmpty(queue) == 0) {
        queuePop(queue);
    }
    //Begin traversing to the leaf nodes in the found hash
    queuePush(queue, currentnode);
    while (queueEmpty(queue) == 0) {
        currentnode = queuePop(queue);
        //If leaf node (chunk) is found
        if(currentnode -> is_leaf == 1) {
            qry.hashes[qry.len] = malloc(SHA256_HEX_LEN + 1);
            strncpy(qry.hashes[qry.len], currentnode -> expected_hash, 64);
            qry.len += 1;
        }
        else {
            queuePush(queue, currentnode -> left);
            queuePush(queue, currentnode -> right);
        }
    }
    free(queue);
    return qry;
}


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry) {
    //Free all contents of the hash array
    for(int i = 0; i < qry -> len; i++) {
        free(qry -> hashes[i]);
    }
    free(qry -> hashes);

}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    //Freeing the chunks array
    if (obj -> chunks) {
        free(obj -> chunks);  
    }

    //Freeing the hashes array
    if (obj -> hashes) {
        for (int i = 0; i < obj -> nhashes; i++) {
            if (obj -> hashes[i]) {
                free(obj -> hashes[i]);
            }
        }

        free(obj -> hashes);
    }
    free(obj);
}
void make_file(struct bpkg_obj* obj) {
    int fd = open(obj -> filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    //Resize the file using truncate
    ftruncate(fd, obj -> size);
    close(fd);
}