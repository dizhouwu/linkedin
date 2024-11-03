#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

const char* MULTICAST_ADDR = "239.0.0.1";
const int PORT = 12345;

int main() {
    int sockfd;
    struct sockaddr_in localAddr;
    struct ip_mreq mreq;
    char buffer[1024];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

    // Allow multiple sockets to use the same PORT number
    int reuse = 1;

    // Set SO_REUSEADDR option
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        close(sockfd);
        return -1;
    }

    // Set SO_REUSEPORT option
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt SO_REUSEPORT failed");
        close(sockfd);
        return -1;
    }

    // Bind to the local address
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return -1;
    }

    // Join the multicast group
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        return -1;
    }

    // Receive messages
    while (true) {
        ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n < 0) {
            perror("recv failed");
            break;
        }
        buffer[n] = '\0'; // Null-terminate the received string
        std::cout << "Received: " << buffer << std::endl;
    }

    close(sockfd);
    return 0;
}
