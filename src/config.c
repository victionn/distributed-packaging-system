#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "../include/config/config.h"
#define MAX_BUFFER 256

int directory_exists(const char* path) {
    struct stat s;
    //If its not a valid path
    if (stat(path, &s) != 0) {
        return 0;
    }
    // Check if the path is a directory
    return S_ISDIR(s.st_mode);
}

void freeConfigObj(struct config_obj* obj) {
    free(obj->directory);
    free(obj);
}
//Function used to verify that the contents in config is valid, and create a directory if it does not exists
void verifyConfigContents(struct config_obj* obj) {
    //Make the directory, and check if it creates, exists, or fail
    //0755 for the folder permissions
    int result = mkdir(obj -> directory, 0755);
    //If directory wasn't created
    if (result != 0) {
        //If directory exists, pass
        if (errno == EEXIST) {;}
        //If the directoy failed to create
        else {
            freeConfigObj(obj);
            exit(3);}
    }
    //Make sure the port and max peer is valid, if not, return with valid error code
    if (!(obj->max_peers > 0 && obj->max_peers < 2049)) {
        freeConfigObj(obj);
        exit(4);
    }
    if (!(obj->port > 1023 && obj->port < 65536)) {
        freeConfigObj(obj);
        exit(5);
    }
}

struct config_obj* config_load(const char* path) {
    //Open the file
    FILE *file = fopen(path, "r");
    if (!file) {return NULL;}
    //Allocate memory for the config object
    struct config_obj* obj = malloc(sizeof(struct config_obj));
    obj -> directory = malloc(MAX_BUFFER);
    char* buffer = malloc(MAX_BUFFER);
    //Begin processing the config file and adding it to the object
    int counter = 0;
    while (fgets(buffer, MAX_BUFFER, file)) {
        if (counter == 0) {sscanf(buffer, "directory:%s\n", obj -> directory);}
        if (counter == 1) {sscanf(buffer, "max_peers:%d\n", &obj -> max_peers);}
        if (counter == 2) {sscanf(buffer, "port:%d\n", &obj -> port);}
        counter ++;
    }
    free(buffer);
    fclose(file);
    //Verify config
    verifyConfigContents(obj);
    return obj;
}