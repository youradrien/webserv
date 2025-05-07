#include "webserv.hpp"

Webserv::Webserv(void)
{
    this->port = PORT;
    this->client_addr_len = sizeof(this->client_addr);

    // create server socket
    this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (this->server_socket < 0)
    {
        throw std::invalid_argument("Error creating socket!");
    }

    // set server socket address (IPv4, localhost, port 8080)
    memset(&this->server_addr, 0, sizeof(this->server_addr));
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_addr.s_addr = INADDR_ANY;
    this->server_addr.sin_port = htons(PORT);

    // bind socket to address and port
    // (turns [address, port] -> [fd] )
    if (bind(this->server_socket, (struct sockaddr*)&this->server_addr, sizeof(this->server_addr)) < 0)
    {
        close(this->server_socket);
        throw std::invalid_argument("Error binding socket!");
    }
}

Webserv::~Webserv()
{
    if (this->client_socket > 0) {
        close(this->client_socket);
        std::cout << "Client socket closed." << std::endl;
    }

    if (this->server_socket > 0) {
        close(this->server_socket);
        std::cout << "Server socket closed." << std::endl;
    }
}


// handle a client request (send a basic HTTP response)
void Webserv::handle_client(int client_socket)
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


void Webserv::start(void)
{
    // start listening on this socket (fd)
    if (listen(server_socket, BACKLOG) < 0)
    {
        close(server_socket);
        throw std::invalid_argument("Error listening on socket");
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // set up poll structure to monitor 1 server socket
    struct pollfd fds[MAX_EVENTS];
    fds[0].fd = server_socket;
    fds[0].events = POLLIN; // watch incoming connections
    while (true)
    {
        // wait for events on the server socket or client sockets
        int ret = poll(fds, 1, -1); // block indefinitely for events
        if (ret < 0)
        {
            std::cerr << "cant in poll()??" << std::endl;
            break;
        }

        // check if incoming connection on server socket
        if (fds[0].revents & POLLIN)
        {
            client_socket = accept(server_socket, (struct sockaddr*)&this->client_addr, &this->client_addr_len);
            if (client_socket < 0) {
                std::cerr << "error accepting cnection!" << std::endl;
                continue;
            }

            std::cout << "accpt new cnection.." << std::endl;
            // handle it 
            this->handle_client(client_socket);
        }
    }
    close(server_socket);
}