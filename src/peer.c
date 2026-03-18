#include "../include/peer/peer.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <signal.h>
#define MAX_EVENTS 100
extern pthread_mutex_t shutdown_mutex;
extern int shutdown_flag;
extern int efd;

void *server_thread(void *arg) {
    int port = *(int*) arg;
    server(port);
    return NULL;
}
void *client_thread(void *arg) {
    struct peer_package* package = (struct peer_package*) arg;
    struct peer_info* peer = package->peer;
    if (client(peer) == 1) {
        //Add the peer to the peer list if connection is successful
        pthread_mutex_lock(&package->peer_list_mutex);
        add_peer(package->peer_list, package->peer);
        pthread_mutex_unlock(&package->peer_list_mutex);
    }
    //If connection was unsuccessful
    else {
        free(peer);
    }
    free(package);
    return NULL;
}

void *server(int port) {
    int server_fd;
    struct sockaddr_in address;

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the addr structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    nonblock(server_fd);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failure");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    //Listen for connection
    if (listen(server_fd, 3) < 0) {
        perror("Listening failure");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    //Create epoll instance
    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS]; //NEEDS TO BE CHANGED TO MAX PEERS
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Error in epoll");
        exit(EXIT_FAILURE);
    }
    //Add server socket to epoll instance
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epool error on server fd");
        close(server_fd);
        close(epoll_fd);
    }
    //Add eventsfd for triggering shutdown
    event.data.fd = efd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, efd, &event) == -1) {
        perror("epoll error on eventfd");
        close(server_fd);
        close(epoll_fd);
        pthread_exit(NULL);
    }
    //Loop to detect incoming transmissions
    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            //If event fd triggers 
            if (events[i].data.fd == efd) {
                uint64_t u;
                read(efd, &u, sizeof(uint64_t));
                pthread_mutex_lock(&shutdown_mutex);
                if (shutdown_flag) {
                    pthread_mutex_unlock(&shutdown_mutex);
                    close(epoll_fd);
                    close(server_fd);
                    pthread_exit(NULL);
                }
                pthread_mutex_unlock(&shutdown_mutex);
            }
        }
    }

    //Listen for incoming connections
    close(epoll_fd);
    close(server_fd);
    pthread_exit(NULL);

    return NULL;
}



//Handles connections to other sockets
int client(struct peer_info* peer) {
    int sockfd;
    struct sockaddr_in address;
    //Socket file descriptor creation
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        printf("Unable to connect to request peer\n");
        free(peer);
        return 0;
    }
    //Set port
    address.sin_family = AF_INET;
    address.sin_port = htons(peer->port);

    //Convert IP address to normal form
    if (inet_pton(AF_INET, peer->ip, &address.sin_addr) <= 0) {
        printf("Unable to connect to request peer\n");
        close(sockfd);
        return 0;
    }

    if (connect(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0 ) {
        printf("Unable to connect to to request peer\n");
        close(sockfd);
        return 0;
    }
    printf("Connection established with peer\n");
    peer->socket_fd = sockfd;
    return 1;

 }

//Set socket to non-blocking mode
void nonblock(int sock) {
    int flag = fcntl(sock, F_GETFL, 0);
    if (flag == -1) {
        perror("Error setting socket to non blocking");
        exit(EXIT_FAILURE);
    }
    flag |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, flag) == -1) {
        perror("Error setting socket to non blocking");
    }



}

struct peer_info* get_peer(struct peer_info* peer_list, char* ip, int port) {
    struct peer_info* current = peer_list;
    while (current != NULL) {
        if (strcmp(current->ip, ip) == 0 && current->port == port) {
            return current;
        }
        current = current -> next;
    }
    return NULL;
}

void add_peer(struct peer_info** peer_list, struct peer_info* peer) {
    // If peer list is empty
    if (*peer_list == NULL) {
        *peer_list = peer;
        return;
    }
    struct peer_info* current = *peer_list; 
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = peer;
}

void free_peer(struct peer_info* peer_list) {
    struct peer_info* currentnode = peer_list;
    struct peer_info* nextnode;
    while(currentnode != NULL) {
        nextnode = currentnode->next;
        close(currentnode->socket_fd);
        free(currentnode);
        currentnode = nextnode;
    }
}

void print_peer(struct peer_info* peer_list) {
    struct peer_info* currentpeer = peer_list;
    int counter = 1;
    printf("Connected to:\n\n");
        while (currentpeer != NULL) {
        printf("%d. %s:%d\n", counter, currentpeer->ip, currentpeer->port);
        currentpeer = currentpeer -> next;
        counter ++;
    }
}

void remove_peer(struct peer_info** peer_list, char* ip, int port) {
    struct peer_info* current = *peer_list;
    struct peer_info* previous = NULL;
    //Iterate through the peer list
    while (current != NULL) {
        //If the ip and port matches a peer
        if (strcmp(current->ip, ip) == 0 && current->port == port) {
            //if this if the first node
            if (previous == NULL) {
                *peer_list = current->next;
            } else {
                previous->next = current->next;
            }
            close(current->socket_fd);
            free(current);
            printf("Disconnected from peer\n");
            return;
        }
        previous = current;
        current = current->next;
    }
    printf("Unknown peer, not connected\n");
}