#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    // Create a UDP socket
    if ((client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Configure server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    std::string message = "Hello from client!";
    
    // Send message to server
    sendto(client_fd, message.c_str(), message.length(), MSG_CONFIRM,
           (const struct sockaddr *)&server_addr, sizeof(server_addr));
    
    std::cout << "Message sent to server" << std::endl;
    
    // Receive response from server
    int len = sizeof(server_addr);
    int n = recvfrom(client_fd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL,
                     (struct sockaddr *)&server_addr, (socklen_t *)&len);
    buffer[n] = '\0';
    
    std::cout << "Received from server: " << buffer << std::endl;
    
    close(client_fd);
    return 0;
}
