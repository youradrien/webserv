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
#include <arpa/inet.h>

#define PORT 8080
#define BACKLOG 10
#define MAX_EVENTS 10

/*
    route / {
        methods GET POST;
        root /var/www/html;
        index index.html;
        autoindex off;
    }

    route /uploads {
        methods POST;
        root /var/www/uploads;
        upload_store /var/www/uploads;
    }

    route /cgi-bin {
        methods GET POST;
        root /var/www/cgi-bin;
        cgi_extension .py;
        cgi_path /usr/bin/python3;
    }

*/
struct LocationConfig 
{
    std::string 
        path,
        root,
        cgi_extension,
        cgi_path,
        index;
    std::vector<std::string> allowed_methods;
    std::string redirect_url;
    bool autoindex;
};

struct ServerConfig
{
    std::string 
        host,
        server_name;
    int 
        port,
        server_socket,
        client_socket;
    struct sockaddr_in 
        server_addr, 
        client_addr;
    socklen_t 
        client_addr_len;
    size_t client_max_body_size;
    std::vector<LocationConfig> locations;
    std::vector<std::pair<unsigned int, std::string> > error_pages;
};

class Webserv
{
    private:
        void handle_client(int client_socket);
        std::vector<ServerConfig> servers;
    public:
        Webserv(void);
        ~Webserv();
        void init(void);
        void start(void);
        bool parseConfigFile(const std::string& filename);
};

#endif