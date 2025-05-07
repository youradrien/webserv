#include "webserv.hpp"

static std::string trim(const std::string& str)
{
    size_t 
        start = str.find_first_not_of(" \t\r\n"),
        end = str.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? 
        "" : 
        str.substr(start, end - start + 1);
}

bool Webserv::parseConfigFile(const std::string& filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        std::cerr << "Could not open config file!" << std::endl;
        return false;
    }

    std::string line;
    std::stack<std::string> context;
    ServerConfig cur_serv;
    LocationConfig serv_loc;

    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line == "server {") 
        {
            context.push("server");
            cur_serv = ServerConfig(); // reset
            
        } else if (line == "location / {" || (  // append new location 
            line.find("location") != std::string::npos && line.find("/") != std::string::npos && 
            line.find("{") != std::string::npos
        ))
        {
            context.push("location");
            serv_loc = LocationConfig(); // reset
            size_t 
                startPos = line.find('/'),
                endPos = line.find('{');
            //std::cout << "MON GROS PAFF " << line.substr(startPos, endPos - startPos - 1) << std::endl;
            serv_loc.path = line.substr(startPos, endPos - startPos - 1);
        } else if (line == "}") 
        {
            if (!context.empty()) 
            {
                if (context.top() == "location")
                    cur_serv.locations.push_back(serv_loc);
                else if (context.top() == "server")
                    this->servers.push_back(cur_serv);
                context.pop();
            }
        } else {
            std::istringstream 
                iss(line);
            std::string 
                key,
                value;
            iss >> key >> value;
            if (!value.empty() && value[value.size()-1] == ';')
                value = value.substr(0, value.size() - 1);

            if (!context.empty())
            {
                if (context.top() == "server")
                {
                    if (key == "host") cur_serv.host = value;
                    else if (key == "port") cur_serv.port = atoi(value.c_str());
                    else if (key == "server_name") cur_serv.server_name = value;
                } else if (context.top() == "location")
                {
                    if (key == "root") serv_loc.root = value;
                    else if (key == "autoindex") serv_loc.autoindex = (value == "on");
                    else if (key == "methods") {
                        serv_loc.allowed_methods.push_back(line.substr(0, line.size() - 1));
                    }
                    else if (key == "index") serv_loc.index = (value);
                }
            }
        }
    }
    file.close();
    std::cout << "\033[92mSUCCESSFULLY PARSED " << servers.size() << " SERVERS ! \033[0m" << std::endl;
    for (size_t i = 0; i < servers.size(); ++i) {
        std::cout << "\033[1;96m - Server [" << i << "]: " << servers[i].host << ":(port)" << servers[i].port << "\033[0m" << std::endl;
        for (size_t j = 0; j < servers[i].locations.size(); ++j) {
            std::cout << " \033[94m     Location: '" << servers[i].locations[j].path << "'\n"
                      << "\t -root: " << servers[i].locations[j].root << "\n"
                      << "\t -index: " << servers[i].locations[j].index << "\n"
                      << "\t -autoindex: " << (servers[i].locations[j].autoindex ? "on" : "off") << "\n"
                      << "\t -methods: ";
                      for (size_t k = 0; k < servers[i].locations[j].allowed_methods.size(); ++k)
                      std::cout << servers[i].locations[j].allowed_methods[k] << std::endl;
                      std::cout << "\033[0m" << std::endl;

        }
    }
    return true;
}