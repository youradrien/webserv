#include "webserv.hpp"

/*
    BRIEF : 
    ✅ Common Socket Syscalls (for a TCP server)

    1️⃣ Create socket	socket()	Creates a new socket file descriptor (FD).
    1. -int fd = socket(AF_INET, SOCK_STREAM, 0);
       -creates a socket using IPv4 (AF_INET) and TCP (SOCK_STREAM).

    2️⃣ Bind address	bind()	Assigns an address (IP + port) to socket.
    2. -bind(fd, (struct sockaddr*)&addr, sizeof(addr));
       -binds socket to an IP and port so  kernel knows where to listen.
    
    3️⃣ Listen for clients	listen()	Marks socket as passive (ready to accept connections).
    3.  -listen(fd, backlog);
        -enables socket to accept incoming connections (backlog = queue size).

    4️⃣ Accept connection	accept()	Accepts an incoming connection, returns a new socket FD for that client.
    4.  -int client_fd = accept(server_fd, NULL, NULL);.
        -accepts client connection. returns new FD for communicating with that client.


    5️⃣ Receive data	recv() / read()	Reads data from client socket.
    5. -recv(client_fd, buffer, sizeof(buffer), 0);
        // or
        read(client_fd, buffer, sizeof(buffer));
       -read bytes from the socket into your program.

    6️⃣ Send data	send() / write()	Sends data back to client socket.
    6. - send(client_fd, response, length, 0);
        // or
        write(client_fd, response, length);
        send(client_fd, response, length, 0);

    7️⃣ Close socket	close()	Closes socket file descriptor.
    7. - close(fd); 
       - closes the socket (server or client).
*/


Webserv::Webserv(void)
{
    std:: cout << "WEBSERVER CAME "<< std::endl;
}

Webserv::~Webserv()
{
    for(uint32_t i = 0; i < this->servers.size(); i ++)
    {
        ServerConfig serv_ = this->servers[i];
        if (serv_.client_socket > 0) {
            close(serv_.client_socket);
            std::cout << "Client socket closed." << std::endl;
        }

        if (serv_.server_socket > 0) {
            close(serv_.server_socket);
            std::cout << "Server socket closed." << std::endl;
        }
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

void Webserv::init(void)
{
    for(uint32_t i = 0; i < this->servers.size(); i ++)
    {
        ServerConfig serv_ = this->servers[i];
        serv_.port = PORT;
        serv_.client_addr_len = sizeof(serv_.client_addr);

        // create server socket
        serv_.server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (serv_.server_socket < 0)
            throw std::invalid_argument("Error creating socket!");
        if(serv_.host.empty() || serv_.port <= 0)
        {
            throw std::invalid_argument("Wrong .conf initialization [ports | domain | methods | root]!");
        }

        // set server socket address (IPv4, localhost, port 8080)
        memset(&serv_.server_addr, 0, sizeof(serv_.server_addr));

        serv_.server_addr.sin_family = AF_INET;
        serv_.server_addr.sin_addr.s_addr = inet_addr(serv_.host.c_str()); // like "127.0.0.1"
        serv_.server_addr.sin_port = htons(serv_.port); // port is 8080
        // bind socket to address and port
        // (turns [address, port] -> [fd] )
        if (bind(serv_.server_socket, (struct sockaddr*)&serv_.server_addr, sizeof(serv_.server_addr)) < 0)
        {
            close(serv_.server_socket);
            throw std::invalid_argument("Error binding socket!");
        }   
    }
}

void Webserv::start(void)
{
    for(uint32_t i = 0; i < this->servers.size(); i ++)
    {
        ServerConfig serv_ = this->servers[i];
        // start listening on serv_. socket (fd)
        if (listen(serv_.server_socket, BACKLOG) < 0)
        {
            close(serv_.server_socket);
            throw std::invalid_argument("Error listening on socket");
        }

        std::cout << "Server" << i << " is listening on port " << PORT << "..." << std::endl;

        // set up poll structure to monitor 1 server socket
        struct pollfd fds[MAX_EVENTS];
        fds[0].fd = serv_.server_socket;
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
                serv_.client_socket = accept(serv_.server_socket, (struct sockaddr*)&serv_.client_addr, &serv_.client_addr_len);
                if (serv_.client_socket < 0)
                {
                    std::cerr << "error accepting cnection!" << std::endl;
                    continue;
                }

                std::cout << "accpt new cnection.." << std::endl;
                // handle it 
                this->handle_client(serv_.client_socket);
            }
        }
        close(serv_.server_socket);
    }
}