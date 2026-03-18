#define MAX_IP 40
#define MAX_PORT 10
#include <pthread.h>

//Structure to hold peer information
struct peer_info {
    char ip[MAX_IP];
    int port;
    int socket_fd;
    struct peer_info *next;
};

//Package used to create a client thread
struct peer_package {
    struct peer_info* peer;
    struct peer_info** peer_list;
    pthread_mutex_t peer_list_mutex;
};

//Used to initialise a server that btide listens to
void* server(int port);
void *server_thread(void *arg);
//Used to make a peer connection
int client(struct peer_info* peer);
void *client_thread(void *arg);
//Sets a socket to non_blocking mode
void nonblock(int sock);
//Adds a peer to the peer linked list
void add_peer(struct peer_info** peer_list, struct peer_info* peer);
//Gets a peer from the linked list
struct peer_info* get_peer(struct peer_info* peer_list, char* ip, int port);
//Free the memory malloced by peer
void free_peer(struct peer_info* peer_list);
//Prints all contents of the peer linked list
void print_peer(struct peer_info* peer_list);
//Removes a peer from the linked list
void remove_peer(struct peer_info** peer_list, char* ip, int port);