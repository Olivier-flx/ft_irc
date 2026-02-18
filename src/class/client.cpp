
#include "Client.hpp"

Client::Client( void ){
	std::cout << "Client : Default Constructor called" << std::endl;
};

Client::Client(int fd, std::string ip)
				:	_fd(fd),
					_ip (ip) {
	std::cout << "Client : Constructor called" << std::endl;
};

Client::Client(std::string nickname, std::string username, std::string password)
				:	_nickname(nickname),
					_username (username),
					_password (password) {
	std::cout << "Client : Constructor called" << std::endl;
};

Client::Client(const Client &cpy) {
	std::cout << "Client : Copy Constructor called" << std::endl;
	*this = cpy;
}

Client &Client::operator=(const Client &src) {
	std::cout << "Client : Overload=  called" << std::endl;
	if (this != &src) {
		_nickname = src._nickname;
		_username = src._username;
		_password = src._password;
		_buffer = src._buffer;
		_isRegistered = src._isRegistered;
	}
	return (*this);
};

Client::~Client() {
	if (_fd != -1) {
		close(_fd);
		_fd = -1;
	}
	std::cout << "Client : Destructor called" << std::endl;
};


void	Client::set_buffer(std::string msg)
{
	_buffer.append(msg);
	std::cout << "Client Buffer : `" << _buffer <<"`\n";
}

std::string		&Client::get_buffer()
{
	return _buffer;
}

void	Client::clear_buffer(size_t start, size_t end)
{
	_buffer.erase(start, end);
	std::cout << "clear_buffer : `" << _buffer <<"`\n";
}

void	Client::set_cmd(std::string cmd) {
	_cmd = cmd;
}

std::string		Client::get_cmd(){
	return _cmd;
}

void	Client::clear_cmd(){
	_cmd.clear();
}
