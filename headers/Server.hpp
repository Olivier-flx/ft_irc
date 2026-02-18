
#ifndef SERVER_HPP
#define SERVER_HPP

#include "ft_irc.hpp"
#include "Client.hpp"

class Server
{
	private:
		Server(void);
		int	_serverSocket;					// La socket d'écoute
		std::string _password;				// Le mdp du serveur
		int	_port;
		std::vector<pollfd> _fds;			// Pour poll()
		std::map<int, Client*> _clients;	// FD -> Pointeur vers Client
//		std::map<string, Channel*> _channels;	// Nom du channel -> Pointeur Channel

		void _exitWithError(const std::string& msg);

	public:

		Server(std::string password, int port);
		Server(const Server &cpy);
		Server &operator=(const Server &src);
		~Server();

		// Methodes
		void	init();				// socket, setsockopt, bind, listen
		void	run();				// Boucle while(true) avec poll()
		void	acceptClient();		// accept() -> new Client
		void	receiveData(int client_fd);	// recv() -> parsing
		void	parsing_msg(int client_fd, std::string msg);
};


#endif
