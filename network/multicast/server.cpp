#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

const char* MULTICAST_ADDR = "239.0.0.1";
const int PORT = 12345;

int main() {
    int sockfd;
    struct sockaddr_in multicastAddr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

    // Set up the multicast address structure
    memset(&multicastAddr, 0, sizeof(multicastAddr));
    multicastAddr.sin_family = AF_INET;
    multicastAddr.sin_addr.s_addr = inet_addr(MULTICAST_ADDR);
    multicastAddr.sin_port = htons(PORT);

    // Loop to send messages
    while (true) {
        const char* message = "Wassup from multicast server!";
        if (sendto(sockfd, message, strlen(message), 0,
                   (struct sockaddr*)&multicastAddr, sizeof(multicastAddr)) < 0) {
            perror("sendto failed");
        } else {
            std::cout << "Sent: " << message << std::endl;
        }
        sleep(1); // Wait for a second
    }

    close(sockfd);
    return 0;
}
