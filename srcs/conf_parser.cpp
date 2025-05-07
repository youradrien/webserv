#include "webserv.hpp"

static std::string trim(const std::string& str)
{
    size_t 
        start = str.find_first_not_of(" \t\r\n"),
        end = str.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

bool Webserv::parseConfigFile(const std::string& filename, std::vector<ServerConfig>& servers)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Could not open config file!" << std::endl;
        return false;
    }

    std::string line;
    std::stack<std::string> context;
    ServerConfig currentServer;
    LocationConfig currentLocation;

    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line == "server {") 
        {
            context.push("server");
            currentServer = ServerConfig(); // reset
        } else if (line == "location / {") {
            context.push("location");
            currentLocation = LocationConfig(); // reset
            currentLocation.path = "/";
        } else if (line == "}") {
            if (!context.empty()) {
                if (context.top() == "location") {
                    currentServer.locations.push_back(currentLocation);
                } else if (context.top() == "server") {
                    servers.push_back(currentServer);
                }
                context.pop();
            }
        } else {
            std::istringstream iss(line);
            std::string key, value;
            iss >> key >> value;
            if (!value.empty() && value[value.size()-1] == ';')
                value = value.substr(0, value.size() - 1);

            if (!context.empty()) {
                if (context.top() == "server") {
                    if (key == "host") currentServer.host = value;
                    else if (key == "port") currentServer.port = atoi(value.c_str());
                    else if (key == "server_name") currentServer.server_name = value;
                } else if (context.top() == "location") {
                    if (key == "root") currentLocation.root = value;
                    else if (key == "autoindex") currentLocation.autoindex = (value == "on");
                }
            }
        }
    }
    file.close();
    return true;
}

/*
     std::vector<ServerConfig> servers;
    
        if (parseConfigFile("webserv.conf", servers)) 
        {
            std::cout << "Parsed " << servers.size() << " server(s)." << std::endl;
            for (size_t i = 0; i < servers.size(); ++i) {
                std::cout << "Server " << i << ": " << servers[i].host << ":" << servers[i].port << std::endl;
                for (size_t j = 0; j < servers[i].locations.size(); ++j) {
                    std::cout << "  Location: " << servers[i].locations[j].path
                              << ", root: " << servers[i].locations[j].root
                              << ", autoindex: " << (servers[i].locations[j].autoindex ? "on" : "off") << std::endl;
                }
            }
        }

*/