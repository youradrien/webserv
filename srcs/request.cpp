#include "request.hpp"
#include "http_form.hpp"
#include "utils.hpp"


Request::~Request()
{ }

Request::Request(char *raw, const ServerConfig &servr, int socket, ssize_t bytes_received)
    : _server(servr), _socket(socket), _bytes_rec(bytes_received)
{
    this->r_header.append(raw, this->_bytes_rec);
    std::istringstream iss(raw);
    std::string buffer;
    iss >> this->r_method >> this->r_location >> this->r_version;

	std::string::size_type pos = this->r_header.find("\r\n\r\n",0);
	if (pos != std::string::npos)
	{
		this->_bytes_rec -= pos + 4;
		std::string body;
		body.append(raw+pos+4, this->_bytes_rec);
		this->r_body = body;
	}

    while (getline(iss, buffer)) {
        if (buffer.size() > 1) {
            std::string key, value;
            std::istringstream iss2(buffer);
            getline(iss2, buffer, ':');
            key = buffer;
            getline(iss2, buffer);
            value = buffer;
            this->http_params.insert(std::make_pair(key, value));
        }
    }
    check_allowed_methods(servr);
}




void Request::check_allowed_methods(const ServerConfig &server)
{
	std::vector<LocationConfig>::const_iterator it_loc = server.locations.begin();
    for(;it_loc != server.locations.end();it_loc++)
    {
		if(it_loc->path == this->r_location)
		{
			this->_loc = *it_loc;
			std::vector<std::string>::const_iterator it_meth = it_loc->allowed_methods.begin();
			for(;it_meth != it_loc->allowed_methods.end();it_meth++)
			{
			    if(this->r_method == *it_meth)
			    {
			        this->authorized = true;
                    this->execute("");
					return ;
			    }
			}
			this->execute("405");
			return ;
		}
    }
	this->execute("404");
}



void Request::execute(std::string s = "null")
{
	if(s == "405") // 405 unallowed method
	{
		const std::string&
				body = readFile("./www/errors/405.html"),
				contentType = "text/html";

		std::string bodi = body;
		// Append extra info to body before calculating Content-Length
		bodi += "<h2>" + this->_loc.root + " only handles:</h2>";
		for (size_t i = 0; i < this->_loc.allowed_methods.size(); ++i)
			bodi += "<h2> - " + this->_loc.allowed_methods[i] + " Method</h2>";

		bodi += "<p>" + this->_loc.index + " only handles: </p>";

		for(unsigned long i = 0; i < this->_loc.allowed_methods.size(); i++)
			bodi += "<p> - " + this->_loc.allowed_methods[i] + " Method</p>";

		HttpForms exec405(this->_socket,405,contentType,bodi,this->_ReqContent);
	}else
	{
		if(!this->authorized)
			return ;
		else if (this->r_method == "GET")
			this->Get();
		else if (this->r_method == "POST")
			this->Post();
		else if (this->r_method == "DELETE")
			this->Delete();
	}
}

void	Request::writeData()
{
	bool parsestate = false;
	if (this->r_boundary =="void")
		return;
	else
	{
		std::istringstream s(this->r_body, std::ios::binary);
		std::string buf;
		while(getline(s,buf))
		{

			if (buf==this->r_boundary + "--\r")
				break;
			else if (buf==this->r_boundary+'\r')
				parsestate = !parsestate;
			else if (parsestate)
			{
				this->file.fname = extract_field_path(buf, "filename=\"");

				getline(s,buf);
				std::string::size_type pos;
				pos = buf.find("Content-Type: ");
				if (pos != std::string::npos)
					this->file.type = buf.substr(pos + 14);
				parsestate = !parsestate;

				getline(s,buf);
				std::string safe_name = sanitize_filename(this->file.fname);
				std::string full_path = this->_loc.upload_store + "/" + safe_name;
				this->file.name = full_path;
				std::ofstream outFile(full_path.c_str(), std::ios::trunc | std::ios::binary);
				if (!outFile)
					throw std::ofstream::failure("aFailed to open file");
			}
			else
			{
				std::string& filename = this->file.name;
				const std::string& content = buf+'\n';
				std::ofstream outFile(filename.c_str(),std::ios::app | std::ios::binary);  // Creates the file if it doesn't exist
				if (!outFile)
					throw std::ofstream::failure("bFailed to open file");
				outFile<<content;
			}

		}
	}
}



void Request::Post()
{
	std::string::size_type pos = this->http_params.find("Content-Type")->second.find("boundary=");
    if (pos != std::string::npos)
	{
        this->r_boundary = this->http_params.find("Content-Type")->second.substr(pos+9);
		this->r_boundary.resize(this->r_boundary.size()-1);
		this->r_boundary = "--" + this->r_boundary;
		if (*this->r_boundary.rbegin() ==' ') this->r_boundary.resize(this->r_boundary.size()-1);
    }
	else
		this->r_boundary = "void";

	ssize_t  content_length = 0;
	if (this->http_params.find("Content-Length") != this->http_params.end())
	{
		content_length = atol(this->http_params["Content-Length"].c_str());
		if (content_length > 10 * 1024 * 1024) { // 10 MB limit
			HttpForms toolarge(this->_socket,413);
			toolarge._sendclose();
			return;
		}
	}

	char buffer[2048];
	while (this->_bytes_rec < content_length)
	{
		ssize_t ret = recv(this->_socket, buffer, sizeof(buffer), 0);
		if (ret == 0)
			break;
		if (ret < 0)
		{
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				continue; // retry the recv
			else {
				std::cerr << "\033[31m [x] post fatal recv err socket: " << this->_socket << " (" << strerror(errno) << ")\033[0m\n";
				HttpForms ServError(this->_socket, 500,"text/plain","Failed to receive POST data.\r\n");
				ServError._sendclose();
				return;
			}
		}
		this->ret = ret;
		this->r_body.append(buffer, ret);
		this->_bytes_rec += ret;
	}
	try
	{
		this->writeData();
		HttpForms ok(this->_socket, 200);
		ok._send();
		std::cout << "\033[32m [✓] POST request handled successfully!\033[0m" << std::endl;
	}
	catch(const std::ofstream::failure& e)
	{
		std::cerr << e.what() << '\n';
	}
}





void Request::Get()
{
	std::string full_path = this->_loc.root;
    std::string file_path;
    struct stat st;

    if (stat(full_path.c_str(), &st) == 0) // 🛠️ REQUIRED!
    {
        if (S_ISDIR(st.st_mode) && (!this->_loc.index.empty() ||
			((&(this->_loc.cgi_extension) != NULL && !this->_loc.cgi_extension.empty())) ))
        {
            file_path = full_path + "/" + this->_loc.index;
        }
        else
        {
            if (this->_loc.autoindex)
                file_path = "[AUTOINDEX]";
            else
                file_path = "[403]";
        }
    }
    else
    	file_path = "[404]";

    if (file_path == "[AUTOINDEX]" )
    {
		std::stringstream listing;
		DIR* dir = opendir(full_path.c_str());
		if (!dir)
			listing << "<li><em>Directory not found: " << full_path << "</em></li>";
		else
		{
			struct dirent* entry;
			while ((entry = readdir(dir)) != NULL)
			{
				std::string name = entry->d_name;
				if (name == "." || name == "..")
					continue;
				listing << "<li><a href=\"" << name << "\">" << name << "</a></li>\n";
			}
			closedir(dir);
		}

		std::string body = readFile("./www/errors/autoindex.html");
		size_t pos = body.find("<!--CONTENT-->");
		if (pos != std::string::npos)
			body.replace(pos, std::string("<!--CONTENT-->").length(), listing.str());
		
		HttpForms notfound(this->_socket, 200,"text/html", body,this->_ReqContent);
	}
	else if (file_path == "[404]")
	{
		std::string path_404 = "./www/default/404.html";
		std::vector<std::pair<unsigned int, std::string> >::const_iterator 
			it = this->_server.error_pages.begin();
		for (; it != this->_server.error_pages.end(); ++it)
		{
			if (it->first == 404)
			{
				path_404 = it->second;
				break;
			}
		}
		const std::string&
				body = readFile(path_404),
				contentType = "text/html";
		HttpForms notfound(this->_socket, 404,contentType, body,this->_ReqContent);
	}
	else if (file_path == "[403]")
	{
		std::string path_403 = "./www/default/403.html";
		std::vector<std::pair<unsigned int, std::string> >::const_iterator 
			it = this->_server.error_pages.begin();
		for (; it != this->_server.error_pages.end(); ++it)
		{
			if (it->first == 403)
			{
				path_403 = it->second;
				break;
			}
		}
		const std::string&
				body = readFile(path_403),
				contentType = "text/html";
		HttpForms forbid(this->_socket, 403, contentType, body,this->_ReqContent);
	}
	else if (file_path == "[REDIRECTION]")
	{
		// todo
		// handle redirection
	}
	else
	{
		if(&(this->_loc.cgi_extension) == NULL || this->_loc.cgi_extension.empty())
		{
			const std::string&
				body = readFile(file_path),
				contentType = "text/html";
			HttpForms ok(this->_socket, 200, contentType, body,this->_ReqContent);
		}else
		{
			std::string script_path;
			char cwd[PATH_MAX];
			if (getcwd(cwd, sizeof(cwd)) == NULL) {
				std::cerr << "getcwd failed" << std::endl;
				return ;
			}
			script_path = std::string(cwd);
			script_path += "/cgi-bin/";
			script_path += findfrstWExtension(script_path, this->_loc.cgi_extension);


			std::cout << script_path << std::endl;
			std::map<std::string, std::string> env;
			env["REQUEST_METHOD"] = this->r_method;
			std::stringstream ss; ss << this->r_body.size();
			env["CONTENT_LENGTH"] = ss.str();
			env["SCRIPT_FILENAME"] = script_path;
			env["CONTENT_TYPE"] = this->http_params["Content-Type"];
			env["GATEWAY_INTERFACE"] = "CGI/1.1";
			env["SERVER_PROTOCOL"] = "HTTP/1.1";
			env["REDIRECT_STATUS"] = "200";


			std::string cgi_output = executeCGI(script_path, this->r_method, this->r_body, env);
			std::string contentType = "text/plain";
			std::string body;
			std::string::size_type header_end = cgi_output.find("\r\n\r\n");
			if (header_end == std::string::npos)
				header_end = cgi_output.find("\n\n");
			if (header_end != std::string::npos)
			{
				std::string headers = cgi_output.substr(0, header_end);
				body = cgi_output.substr(header_end + 4);
				std::istringstream headerStream(headers);
				std::string line;
				while (std::getline(headerStream, line))
				{
					if (line.find("Content-Type:") != std::string::npos)
					{
						contentType = line.substr(line.find(":") + 1);
						while (contentType[0] == ' ') contentType.erase(0, 1);
					}
				}
			}
			else
				body = cgi_output; // no headers? treat all as body
			HttpForms ok(this->_socket, 200, contentType, body,this->_ReqContent);
			std::cout << this->_ReqContent << std::endl;
		}
	}
}




void Request::Delete()
{
	// make sure that delete only runs into the upload/ path
	char buf[PATH_MAX];
	std::string f_path;
	if (this->http_params.find("X-Filename") != this->http_params.end() 
		&& this->http_params["X-Filename"].length() != 0 && getcwd(buf, sizeof(buf)))
	{
	    struct stat buffer;
		const std::string &full_path = std::string(buf)
				+ this->_loc.upload_store.substr(1)
				+ "/"
				+ trim(this->http_params["X-Filename"]);
		if(stat(full_path.c_str(), &buffer) != 0)
		{
			std::cerr << "\033[31m[not found]: " << full_path << "\033[0m"<< std::endl;
			HttpForms notfound(this->_socket, 404);
			notfound._send();
			return;
		}else
		{
			std::cerr << "\033[32m[successfully found]: " << full_path << "\033[0m"<< std::endl;
			f_path = (full_path);
		}
	}else
	{
		HttpForms Badreq(this->_socket, 400);
		Badreq._sendclose();
		return;
	}

    // delete file
    if (std::remove(f_path.c_str()) == 0)
    {
		HttpForms ok(this->_socket,200, "text/plain","success");
		ok._send();
		return;
    }
    else
    {
		HttpForms notfound(this->_socket,404);
		notfound._send();
    }
    close(this->_socket);
}


std::string Request::_get_content()
{
	return this->_ReqContent;
}

std::string Request::_get_header(const std::string &key) const {
    std::map<std::string, std::string>::const_iterator it = http_params.find(key);
    if (it != http_params.end())
        return it->second;
    return "";
}

std::string Request::_get_httpversion() const {
    return r_version;
}