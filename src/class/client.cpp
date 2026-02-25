
#include "Client.hpp"

Client::Client( void )
{
	this->_fd = -1;
	this->_ip = "";
	this->_nickname = "";
	this->_username = "";
	this->_buffer = "";
	this->_isOperator= false;
	this->_isRegistered = false;
	this->_Authenticated = false;
}

Client::Client(std::string nickname, std::string username, int fd): _fd(fd), _nickname(nickname), _username(username){}

Client::Client(const Client &cpy) { *this = cpy; }

Client &Client::operator=(const Client &src) 
{
	if (this != &src) 
	{
		this->_nickname = src._nickname;
		this->_username = src._username;
		this->_fd = src._fd;
		this->_ip = src._ip;
		this->_buffer = src._buffer;
		this->_isOperator= src._isOperator;
		this->_isRegistered = src._isRegistered;
		this->_Authenticated = src._Authenticated;
	}
	return (*this);
};

Client::~Client(){}

//getters


int Client::GetFd() const { return (this->_fd); }   

const std::string& Client::GetNickname() const { return (this->_nickname); }

const std::string& Client::GetUsername() const { return (this->_username); }

const std::string& Client::GetBuffer() const { return (this->_buffer); }

const std::string& Client::GetIpAdd() const { return (this->_ip); }

std::string Client::GetHostname() const { return (_nickname + "!" + _username); }

bool Client::IsRegistered() const { return (_isregistered); }

bool Client::IsAuthenticated() const { return (_Authenticated); }


//---------------//Setters


void Client::clearBuffer()
{ 
	buffer.clear(); 
}

