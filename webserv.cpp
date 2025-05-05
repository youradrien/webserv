#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#define PORT 8080
#define BACKLOG 10
#define MAX_EVENTS 10

// Handle a client request (send a basic HTTP response)
void handle_client(int client_socket)
{
    char buffer[1024];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0)
    {
        std::cerr << "Error receiving data from client!" << std::endl;
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0'; // null-terminate received data

    // log  received HTTP request
    std::cout << "Received request:\n" << buffer << std::endl;

    // Construct a basic HTTP response
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "<html><body><h1>Hello, World!</h1></body></html>\r\n";
    send(client_socket, response, strlen(response), 0);

    close(client_socket); // Close the client socket after sending the response
}

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating socket!" << std::endl;
        return -1;
    }

    // Set server socket address (IPv4, localhost, port 8080)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket!" << std::endl;
        close(server_socket);
        return -1;
    }

    // Start listening for incoming connections
    if (listen(server_socket, BACKLOG) < 0) {
        std::cerr << "Error listening on socket!" << std::endl;
        close(server_socket);
        return -1;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // Set up the poll structure to monitor the server socket
    struct pollfd fds[MAX_EVENTS];
    fds[0].fd = server_socket;
    fds[0].events = POLLIN; // Watch for incoming connections

    while (true)
    {
        // Wait for events on the server socket or client sockets
        int ret = poll(fds, 1, -1); // Block indefinitely for events
        if (ret < 0) {
            std::cerr << "Error in poll()" << std::endl;
            break;
        }

        // Check if there is an incoming connection on the server socket
        if (fds[0].revents & POLLIN) {
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_socket < 0) {
                std::cerr << "Error accepting connection!" << std::endl;
                continue;
            }

            std::cout << "Accepted new connection..." << std::endl;

            // Handle the client request
            handle_client(client_socket);
        }
    }

    // Clean up
    close(server_socket);
    return 0;
}