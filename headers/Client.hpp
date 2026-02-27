
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "ft_irc.hpp"

class Client
{
	private:
		Client(void); 
		int			_fd; // socket individuelle d'un client
		std::string	_ip;
		std::string	_nickname;
		std::string	_username;
		std::string	_hostname;
		std::string	_password;
		std::string	_buffer;                // ⚠️ CRUCIAL : Stocke les morceaux de msg
		bool 		_Authenticated; //a envoyé le bon username, nickname et password
		bool		_isRegistered; // a envoyé nick + user + pass ET a un fd (= a été accepée par le serveur)
		bool 		_isOperator;


	public:
		Client(std::string nickname, std::string username, int fd) //fd au lieu de password car on connait le fd au moment où on accepte la connexion mais pas encore le password
		Client(const Client &cpy);
		Client &operator=(const Client &src);
		~Client();

		//getters

		int Client::getFd() const;  
		const std::string& getNickname() const;
		const std::string& getUsername() const;
		const std::string& getBuffer() const;
		const std::string& getIp() const;
		std::string getHostname() const;

		bool isRegistered() const;
		bool isAuthenticated() const;
		bool isOperator() const;


		//setters

		void setNickname(const std::string &nickname);
		void setUsername(const std::string &username);
		void setBuffer(std::string &data);
		void setFd(int fd);
		void setOperator(bool value);
		void setRegistered(bool value);
		void setIpAdd(const std::string &ipadd);
		void appendBuffer(const std::string& data);


#endif
