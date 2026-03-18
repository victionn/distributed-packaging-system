#include "../include/net/packet.h"
#include <sys/socket.h>
#include <sys/types.h>

ssize_t send_btide_packet(int sockfd, const struct btide_packet *packet) {
    return send(sockfd, packet, sizeof(struct btide_packet), 0);
}

ssize_t recv_btide_packet(int sockfd, struct btide_packet *packet) {
    return recv(sockfd, packet, sizeof(struct btide_packet), 0);
}