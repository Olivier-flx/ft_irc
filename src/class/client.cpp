
#include "Client.hpp"

Client::Client( void )
{
	this->_fd = -1; //car fd valid >=0 
	this->_ip = "";
	this->_nickname = "";
	this->_username = "";
	this->_buffer = "";
	this->_Authenticated = false;
	this->_isOperator= false;
	this->_isRegistered = false;
}

Client::Client(std::string nickname, std::string username, int fd): _nickname(nickname), _username(username), _fd(fd), {}

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
		this->_Authenticated = src._Authenticated;
		this->_isOperator= src._isOperator;
		this->_isRegistered = src._isRegistered;
	}
	return (*this);
};

Client::~Client(){}

//getters


int Client::getFd() const { return (this->_fd); }   

const std::string& Client::getNickname() const { return (this->_nickname); }

const std::string& Client::getUsername() const { return (this->_username); }

const std::string& Client::getBuffer() const { return (this->_buffer); }

const std::string& Client::getIp() const { return (this->_ip); }

std::string Client::getHostname() const { return (_nickname + "!" + _username); }

bool Client::isRegistered() const { return (_isRegistered); }

bool Client::isAuthenticated() const { return (_Authenticated); }

bool Client::isOperator() const { return (this->isOperator); }


//---------------//Setters

void Client::setNickname(const std::string &nickname){ this->_nickname = nickname; }

void Client::setUsername(const std::string &username){ this->_username = username; }

void Client::setFd(int fd){ this->_fd = fd; }

void Client::setOperator(bool value){ _isOperator = value; }

void Client::appendBuffer(const std::string& data){ _buffer += data; }

void Client::setRegistered(bool value) { _isRegistered = value; }

void Client::setIpAdd(const std::string &ipadd) { this->_ip = ipadd; }


void Client::clearBuffer()
{ 
	_buffer.clear(); 
}

