
#ifndef SERVER_HPP
#define SERVER_HPP

#include "ft_irc.hpp"
#include "Client.hpp"

class Server
{
	private:
		Server(void);
		int	_serverSocket;				// La socket d'écoute
		int	_port;
		std::string _password;				// Le mdp du serveur
		static bool _Signal; // for signalhandle()
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
		void	receiveData(int fd);	// recv() -> parsing
		void	close_fds();

		//Parsing
		bool nicknameUsed(std::string& nickname);
		bool isValidNickname(std::string& nickname);

		static void SignalHandler(int signum); // signal handler
};

#endif
