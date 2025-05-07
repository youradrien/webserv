#ifndef WEBSERV_HPP

# define WEBSERV_HPP

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>  // for std::invalid_argument
#include <vector>
#include <map> 
#include <stack> 
#include <sstream>

#define PORT 8080
#define BACKLOG 10
#define MAX_EVENTS 10

struct LocationConfig 
{
    std::string path;
    std::string root;
    std::vector<std::string> allowed_methods;
    std::string redirect_url;
    bool autoindex;
};

struct ServerConfig {
    std::string host;
    int port;
    std::string server_name;
    std::vector<LocationConfig> locations;
};

class Webserv
{
    private:
        void handle_client(int client_socket);
        int 
            server_socket,
            client_socket,
            port;
        struct sockaddr_in 
            server_addr, 
            client_addr;
        socklen_t 
            client_addr_len;
        std::string 
            host,
            server_name;
        std::map<int, std::string> 
            error_pages;
        size_t 
            client_max_body_size;
        std::vector<LocationConfig>
            locations;
        std::vector<ServerConfig>
            servers;
    public:
        Webserv(void);
        ~Webserv();
        void start(void);
        bool parseConfigFile(const std::string& filename);
};

#endif