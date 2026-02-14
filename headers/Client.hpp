
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "ft_irc.hpp"

class Client
{
	private:
		Client(void); // unauthorised default constructor
		int			_fd;
		std::string	_ip;
		std::string	_nickname;
		std::string	_username;
		std::string	_password;
		std::string	_buffer;                // ⚠️ CRUCIAL : Stocke les morceaux de msg
		bool		_isRegistered;

	public:
		Client (int client_fd, std::string client_ip);
		Client(std::string nickname, std::string username, std::string password);
		Client(const Client &cpy);
		Client &operator=(const Client &src);
		~Client();
};

#endif
