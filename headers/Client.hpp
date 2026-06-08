
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
		std::string	_password;
		std::string	_buffer;   // Stocke les morceaux de msg
		std::string	_cmd;
		bool 		_Authenticated; //a envoyé le bon password
		bool		_isRegistered; // a envoyé nick + user + pass ET a un fd (= a été accepée par le serveur)
		bool 		_isOperator; //droit speciaux sur le serveur entier (KILL, OPER, .. )

	public:
		Client (int client_fd, std::string client_ip);
		Client(std::string nickname, std::string username, int fd); //[NON UTILISE] //fd au lieu de password car on connait le fd au moment où on accepte la connexion mais pas encore le password
		Client(const Client &cpy);
		Client &operator=(const Client &src);
		~Client();


		//[IDENTITE]
			// getters
			int					getFd() const;
			const std::string	&getIp() const;
			const std::string	&getNickname() const;
			const std::string	&getUsername() const;
			const std::string	getHostname() const;
			std::string         getPrefix() const;

			// setters
			void				setFd(int fd);
			void 				setIpAdd(const std::string &ipadd);
			void				setNickname(const std::string &nickname);
			void				setUsername(const std::string &username);

		//[ETAT]
			// getters
			bool				isRegistered() const;
			bool				isAuthenticated() const;
			bool				isOperator() const;

			// setters
			void				setOperator(bool value);
			void 				setAuthenticated(bool value);
			void				setRegistered(bool value);

		//[BUFFER & COMMANDS]
			// getters
			std::string			&getBuffer();
			std::string			getCmd() const;

			// setters
			void				setCmd(const std::string &cmd);

			// methodes
			void				appendBuffer(const std::string &data);
			void				clearBuffer(size_t start, size_t end);
			void				clearCmd();
};

#endif
