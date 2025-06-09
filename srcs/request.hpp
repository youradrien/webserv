#ifndef REQUEST_HPP

# define REQUEST_HPP

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
#include <filesystem>
#include <sys/stat.h>
#include <algorithm>
#include "webserv.hpp"
#include <dirent.h>
#include <string>

class Request
{
    public:
      Request(char* buffer);
      Request(char* buffer, const ServerConfig &serv);
      ~Request();
      void check_allowed_methods(const ServerConfig &serv);
      void execute(std::string s);
      std::string _get_ReqContent();

    private:
        std::string r_method, r_location, r_version, r_full_request;
        LocationConfig _loc;
        bool authorized;
        void Post();
        void Get();
        void Put();
        void Patch();
        void Delete();
        void Head();
        void Options();
        std::string _ReqContent;

};

#endif