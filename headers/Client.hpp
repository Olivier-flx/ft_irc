
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "ft_irc.hpp"

class Client
{
	private:
		Client(void); // unauthorised default constructor
		int			_fd; // socket individuelle d'un client
		std::string	_ip;
		std::string	_nickname;
		std::string	_username;
		std::string	_hostname;
		std::string	_password;
		std::string	_buffer;                // ⚠️ CRUCIAL : Stocke les morceaux de msg
		bool		_isRegistered;
		bool 		_isOperator;
		bool 		_Authenticated; //logged (username, nickename, password)

	public:
		Client(std::string nickname, std::string username, int fd) //fd au lieu de password car on connait le fd au moment où on accepte la connexion mais pas encore le password
		Client(const Client &cpy);
		Client &operator=(const Client &src);
		~Client();

		//getters

		int Client::GetFd() const;  
		const std::string& GetNickname() const;
		const std::string& GetUsername() const;
		const std::string& GetBuffer() const;
		const std::string& GetIpAdd() const;
		std::string GetHostname() const;

		bool IsRegistered() const;
		bool IsAuthenticated() const;


		//setters

		void setNickname(std::string &nickname);
		void setUsername(std::string &username);
		void setBuffer(std::string &data);

		void clearBuffer();

};

#endif
