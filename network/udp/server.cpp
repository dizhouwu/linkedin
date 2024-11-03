#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    
    // Create a UDP socket
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Configure server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind the socket to the specified port
    if (bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }
    
    std::cout << "Server listening on port " << PORT << std::endl;
    
    socklen_t len = sizeof(client_addr);
    int n;
    
    // Server loop to receive messages
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        
        // Receive message from client
        n = recvfrom(server_fd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, 
                     (struct sockaddr *)&client_addr, &len);
        buffer[n] = '\0';
        
        std::cout << "Received from client: " << buffer << std::endl;
        
        // Send response to client
        std::string response = "Hello from server!";
        sendto(server_fd, response.c_str(), response.length(), MSG_CONFIRM,
               (const struct sockaddr *)&client_addr, len);
        
        std::cout << "Response sent to client" << std::endl;
    }
    
    close(server_fd);
    return 0;
}
