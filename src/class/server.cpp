
#include "Server.hpp"

Server::Server( void ){
	std::cout << "Server : Default Constructor called" << std::endl;
};

Server::Server(int serverSocket, std::string password)
				:	_serverSocket(serverSocket),
					_password (password)
{
	std::cout << "Server : Constructor called" << std::endl;
};

Server::Server(const Server &cpy) {
	std::cout << "Server : Copy Constructor called" << std::endl;
	*this = cpy;
}

Server &Server::operator=(const Server &src) {
	std::cout << "Server : Overload=  called" << std::endl;
	if (this != &src) {
		_serverSocket = src._serverSocket;
		_fds = src._fds;
		_clients = src._clients;
		//_channels = src._channels;
		_password = src._password;
	}
	return (*this);
};

Server::~Server() {
	std::cout << "Server : Destructor called" << std::endl;
};

///////////// Methodes //////////

void	Server::init()
{
	std::cout << "init() \n";
};				// socket, setsockopt, bind, listen
void	Server::run()
{
	std::cout << "run() \n";
};				// Boucle while(true) avec poll()
void	Server::acceptClient()
{
	std::cout << "acceptClient() \n";
};		// accept() -> new Client
void	Server::receiveData(int fd)
{
	std::cout << "receiveData() \n";
};	// recv() -> parsing

