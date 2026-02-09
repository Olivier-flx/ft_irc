
#include "Client.hpp"

Client::Client( void ){
	std::cout << "Client : Default Constructor called" << std::endl;
};

Client::Client(std::string nickname, std::string username, std::string password)
				:	_nickname(nickname),
					_username (),
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
	std::cout << "Client : Destructor called" << std::endl;
};
