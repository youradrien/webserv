#ifndef CLIENT_HPP
# define CLIENT_HPP


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
#include <dirent.h>
#include <string>
#include "webserv.hpp"
#include <cstdio>
#include <csignal>

#define WAITING 0
#define READING 1
#define READINGDATA 2
#define WRITING 3
#define CLOSING 4
#define DONE 5
#define READINGPOSTDATA 6
#define READINGHEADER 7
#define EXECUTING 8
#define DELETE 9
#define SENDERROR 10


class HttpForms;
class Request;
class CGIHandler;

class client
{
	public:
		client(){};
		client( pollfd& pfdin, ServerConfig *server);
		~client();

		HttpForms *_formulaire;
		Request	*_request;
		
		//METHODS
		void tryLaunchCGI();
		bool handle_jesus(pollfd& pfdin);
		bool answerClient(pollfd& pfdin);
		int getStatus()const
			{return status;};
		int getFd()const
			{return fd;};
		int 
			status, 
			cgi_fd;
		bool keepalive, cgiresgitered;
		std::string cgi_buffer;
		CGIHandler* cgi_handler;
		
	private:
		ServerConfig *serv;
		int 
			fd, 
			pollstatus;
		std::string data;
		ssize_t	totalrec;
};


#endif