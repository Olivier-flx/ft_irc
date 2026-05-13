#ifndef SERVER_HPP
#define SERVER_HPP

#include "ft_irc.hpp"
#include "Client.hpp"
#include "Channel.hpp"

class Server
{
	private:
		Server(void);
		int	_serverSocket;				// La socket d'écoute
		int	_port;
		std::string _password;				// Le mdp du serveur
		static const std::string _serverName;
		static bool _Signal; // for signalhandle()
		std::vector<pollfd> _fds;			// Pour poll()
		std::map<int, Client*> _clients;	// FD -> Pointeur vers Client
		std::map<std::string, Channel*> _channels;	// Nom du channel -> Pointeur Channel
		std::string _creationTime;

		void _exitWithError(const std::string& msg);

	public:

		Server(int port, std::string password);
		Server(const Server &cpy);
		Server &operator=(const Server &src);
		~Server();

		// Methodes
		void	init();				// socket, setsockopt, bind, listen
		void	run();				// Boucle while(true) avec poll()
		void	acceptClient();		// accept() -> new Client
		bool	receiveData(int client_fd);	// recv() -> parsing
		bool	get_line_msg_to_cmd(int client_fd);
		void	parse_cmd(int client_fd);
		void	exec_cmd(int client_fd);
		void	client_disconnection(size_t i);

		void	close_fds();

		static void SignalHandler(int signum); // signal handler

		//Authentification
		bool	nicknameUsed(std::string& nickname);
		bool	isValidNickname(std::string& nickname);

		void	handlePass(int fd, std::istringstream &iss);
		void	handleNick(int fd, std::istringstream &iss);
		void	handleUser(int fd, std::istringstream &iss);
		void	tryRegister(int fd);
		const	std::string& getCreationTime() const;
		void	sendMessage(int fd, const std::string &msg);

		//Commands
		void	handleJoin(int fd, std::istringstream &iss);
		bool	cannotJoinChannel(Channel *ch, Client *client, const std::string &key);

		void	handlePrivMsg(int fd, std::istringstream &iss);
		void	handleTopic(int fd, std::istringstream &iss);
		void	handleMode(int fd, std::istringstream &iss);
		void	handlePart(int fd, std::istringstream &iss);
		void	handleInvite(int fd, std::istringstream &iss);
		void	handleKick(int fd, std::istringstream &iss);
		void	handlePing(int fd, std::istringstream &iss);
		Client*	getClientByNick(const std::string& nick);


};


#endif
