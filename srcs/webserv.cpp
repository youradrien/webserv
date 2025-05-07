#include "webserv.hpp"

int main(int argc, char **argv)
{
    Webserv w;
    // create server
    try{
        w = Webserv();  // Invalid input
    } catch (const std::invalid_argument& e)
    {
        std::cerr << "\033[33mError creating the server: \033[0m" << e.what() << std::endl;
    }
    // start the server
    try{
        w.start();  // Invalid input
    } catch (const std::invalid_argument& e)
    {
        std::cerr << "\033[33m Error starting the server: \033[0m" << e.what() << std::endl;
    }
    return 0;
}