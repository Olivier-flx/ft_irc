
#ifndef SERVER_HPP
#define SERVER_HPP

#include "ft_irc.hpp"
#include "Client.hpp"

class Server
{
	private:
		Server(void); // unauthorised default constructor
		int _serverSocket;					// La socket d'écoute
		std::vector<pollfd> _fds;			// Pour poll()
		std::map<int, Client*> _clients;	// FD -> Pointeur vers Client
//		std::map<string, Channel*> _channels;	// Nom du channel -> Pointeur Channel
		std::string _password;				// Le mdp du serveur

	public:
		Server(int serverSocket, std::string password);
		Server(const Server &cpy);
		Server &operator=(const Server &src);
		~Server();

		// Methodes
		void	init();				// socket, setsockopt, bind, listen
		void	run();				// Boucle while(true) avec poll()
		void	acceptClient();		// accept() -> new Client
		void	receiveData(int fd);	// recv() -> parsing
};

#endif
