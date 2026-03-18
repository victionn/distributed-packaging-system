#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include "../include/config/config.h"
#include "../include/peer/peer.h"
#include <ctype.h>
#include "../include/package/package.h"
#define MAX_INPUT 5520
#define MAX_BUFFER 256
pthread_mutex_t peer_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shutdown_mutex = PTHREAD_MUTEX_INITIALIZER;
int shutdown_flag = 0;
int main_loop = 1;
int efd;

int validIP(char* ipport) {
    char* colon = strchr(ipport, ':');
    //if : is not specified
    if (colon == NULL || colon == ipport || *(colon + 1) == '\0') {
        return 0;
    }
    char ip[MAX_IP];
    char portstring[MAX_PORT];
    // Get the ip and port
    strncpy(ip, ipport, colon - ipport);
    ip[colon - ipport] = '\0';
    strcpy(portstring, colon + 1);
    // If ip or port is not specified
    if(strlen(ip) == 0 || strlen(portstring) == 0) {
        return 0;
    }
    int port = 0;
    //If the ip or port is not a number
    if (sscanf(portstring, "%d", &port) != 1) {
        return 0;
    }
    return 1;

}

void extract_file(char* processedfile, char* file) {
    int j = 0;
    for (int i = 0; processedfile[i] != '\0'; i++) {
        //If file begins with ./
        if (processedfile[i] == '.' && processedfile[i + 1] == '/') {
            i++;
            continue;
        }
        //Only store the digit and alphabeitcal characters to the name
        if (isalpha(processedfile[i]) || isdigit(processedfile[i]) || (processedfile[i] == '.' && i > 0 && processedfile[i-1] != '/')) {
            file[j++] = processedfile[i];
        }

    }
    file[j] = '\0';
}
void process_command(struct peer_info** peer_list, struct config_obj* config, struct bpkg_list** bpkg_list, pthread_t server_tid) {

    char input[MAX_INPUT];
    if (fgets(input, MAX_INPUT, stdin) == NULL) {
        exit(EXIT_SUCCESS);
    }
    //If user enters QUIT, exit the program
    if(strncmp(input, "QUIT", 4) == 0) {
        main_loop = 0;
        pthread_mutex_lock(&shutdown_mutex);
        shutdown_flag = 1;
        pthread_mutex_unlock(&shutdown_mutex);
        //Trigget eventfd to wake up epoll_wait
        uint64_t u = 1;
        write(efd, &u, sizeof(uint64_t));
        pthread_join(server_tid, NULL);
        pthread_mutex_lock(&peer_list_mutex);
        //Free all allocated memory
        free_peer(*peer_list);
        free(config -> directory);
        free(config);
        free_package(*bpkg_list);
        pthread_mutex_unlock(&peer_list_mutex);
        return;
    }
    else if(strncmp(input, "PEERS", 5) == 0) {
        //If there are no peers
        if (*peer_list == NULL) {
            printf("Not connected to any peers\n");
        }
        else {
            print_peer(*peer_list);
            
        }
        
    }
    else if(strncmp(input, "ADDPACKAGE", 10) == 0) {
        char processedfile[MAX_BUFFER];
        char file[MAX_BUFFER];
        //If no arguments are provided
        if (sscanf(input, "ADDPACKAGE %s", processedfile) != 1) {
            printf("Missing file argument\n");
            return;
        }
        //Remove ./ from the file name
        extract_file(processedfile, file);

        if(strlen(file) == 0) {
            printf("Missing file argument\n");
            return;
        }

        add_package(config -> directory, file, bpkg_list);
        return;
    }

    else if(strncmp(input, "PACKAGES", 8) == 0) {
        print_package(*bpkg_list);
    }

   else if(strncmp(input, "REMPACKAGE", 10) == 0) {
        char identifier[1025];
        if (sscanf(input, "REMPACKAGE %s", identifier) != 1) {
            printf("Missing identifier amount, please specify whole 1024 characters or at least 20 characters.\n");
            return;
        }
        if(strlen(identifier) == 0) {
            printf("Missing identifier amount, please specify whole 1024 characters or at least 20 characters.\n");
            return;
        }
        remove_package(bpkg_list, identifier);
    }

    else if (strncmp(input, "CONNECT", 7) == 0) {
        char ipport[MAX_BUFFER];
        sscanf(input, "CONNECT %s", ipport);
        //If IP is invalid
        if (validIP(ipport) == 0) {
            printf("Missing address and port argument\n");
            return;
        }
        // Get the ip and port
        char* colon = strchr(ipport, ':');
        char ip[MAX_IP];
        char portstring[MAX_PORT];
        strncpy(ip, ipport, colon - ipport);
        ip[colon - ipport] = '\0';
        strcpy(portstring, colon + 1);
        int port = atoi(portstring);

        //Check if the peer already exists
        pthread_mutex_lock(&peer_list_mutex);
        struct peer_info* existing_peer = get_peer(*peer_list, ip, port);
        if (existing_peer != NULL) {
            printf("Error: Connection already made\n");
            pthread_mutex_unlock(&peer_list_mutex);
            return;
        }
        pthread_mutex_unlock(&peer_list_mutex);
        //Initliase peer structure
        struct peer_info* peer = malloc(sizeof(struct peer_info));
        if (peer == NULL) {
            perror("malloc");
        }
        strcpy(peer -> ip, ip);
        peer -> port = port;
        peer -> next = NULL;
        //Initialise peer package
        struct peer_package* package = malloc(sizeof(struct peer_package));
        if (package == NULL) {
            perror("malloc");
            free(peer);
            return;
        }
        package -> peer = peer;
        package -> peer_list = peer_list;
        package -> peer_list_mutex = peer_list_mutex;
        //Create thread to handle the connection
        pthread_t client_tid;
        if (pthread_create(&client_tid, NULL, client_thread, (void*) package) != 0) {
            perror("pthread_create");
            free(peer);
            free(package);
        }
        //Detach the pthread to run it independently
        pthread_detach(client_tid);
    } 
    else if (strncmp(input, "DISCONNECT", 7) == 0) {
        char ipport[MAX_BUFFER];
        sscanf(input, "DISCONNECT %s", ipport);
        //If IP is invalid
        if (validIP(ipport) == 0) {
            printf("Missing address and port argument\n");
            return;
        }
        // Get the ip and port
        char* colon = strchr(ipport, ':');
        char ip[MAX_IP];
        char portstring[MAX_PORT];
        strncpy(ip, ipport, colon - ipport);
        ip[colon - ipport] = '\0';
        strcpy(portstring, colon + 1);
        int port = atoi(portstring);
        //Remove the peer
        pthread_mutex_lock(&peer_list_mutex);
        remove_peer(peer_list, ip, port);
        pthread_mutex_unlock(&peer_list_mutex);

    }
    else {
        printf("Invalid Input.\n");
    }
}

int main(int argc, char** argv) {
    //Initialise peer and bpkg list

    struct peer_info* peer_list = NULL;
    struct bpkg_list* bpkg_list = NULL;

    //Load in the config
    char* path = argv[1];
    struct config_obj* config = config_load(path);
    if (config == NULL) {
        free(config->directory);
        free(config);
        return 1;
    }
    int *port = &config -> port;
    //Create eventfd for waking up epoll_wait
    efd = eventfd(0, 0);
    if (efd == 1) {
        perror("eventfd");
        exit(EXIT_FAILURE);
    }
    //Create the server thread
    pthread_t server_tid;
    pthread_create(&server_tid, NULL, server_thread, (void *)port);
    while (main_loop) {
        process_command(&peer_list, config, &bpkg_list, server_tid);
    }
    close(efd);
    return 0;
}