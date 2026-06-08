#include "ft_irc.hpp"
#include "Server.hpp"

const std::string Server::_serverName = "ft_irc";

Server::Server(int port, std::string password )
				:	_port(port),
					_password (password)
{
	_Signal = false;

	time_t t = time(NULL);
	_creationTime = ctime(&t);		// récupère date/heure en string
	if (!_creationTime.empty() && _creationTime[_creationTime.length() - 1] == '\n')
		_creationTime.erase(_creationTime.length() - 1);		// supprime le '\n' ajouté par ctime

	std::cout << "Server : Constructor called" << std::endl;
};

Server::Server(const Server &cpy) {
	std::cout << "Server : Copy Constructor called" << std::endl;
	*this = cpy;
}

Server &Server::operator=(const Server &src) {
	std::cout << "Server : Overload=  called" << std::endl;
	if (this != &src) {
		_serverSocket = src._serverSocket;
		_port = src._port;
		_fds = src._fds;
		_clients = src._clients;
		_channels = src._channels;
		_password = src._password;
	}
	return (*this);
};

Server::~Server()
{
	if (_serverSocket != -1) {
		close(_serverSocket);
		_serverSocket = -1;
	}

	std::map<int, Client*>::iterator it;
	for (it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->second != NULL)
			delete it->second;
	}
	_clients.clear();
	//While () fd ouvert, fermer tous ls fd ces clients

	std::map<std::string, Channel*>::iterator it2;//nettoie channels
	for (it2 = _channels.begin(); it2 != _channels.end(); ++it2)
	{
		if (it2->second)
			delete it2->second;
	}
	_channels.clear();

	std::cout << "Server : Destructor called" << std::endl;
};

const std::string& Server::getCreationTime() const { return _creationTime; }

volatile sig_atomic_t Server::_Signal = 0; //-> initialize the static boolean

void Server::SignalHandler(int signum) //met variable à true en cas de reception d'un signal
{
	(void)signum;
	Server::_Signal = 1;
}

void Server::_exitWithError(const std::string& msg)
{
	if (_serverSocket != -1) {
		close(_serverSocket);
		_serverSocket = -1;
	}
	throw std::runtime_error(msg + ": " + std::strerror(errno));
}

void Server::close_fds()
{
	std::map<int, Client*>::iterator it;
	for (it = _clients.begin(); it != _clients.end(); ++it)
	{
		int fd = it->first;
		Client* obj = it->second;

		std::cout << "Client " << fd << " (Nick: " << obj->getNickname() << ") > Disconnected" << std::endl;
		sendMessage(it->first, "ERROR :Server shutting down"); // to avoid auto reconnections
		close(fd);
		delete obj; // libèration de la mémoire allouée (le Client*)
	}
	_clients.clear();

	if (_serverSocket != -1)
	{
		std::cout << "Server (FD: " << _serverSocket << ") > Shutting down" << std::endl;
		close(_serverSocket);
		_serverSocket = -1;
	}
}


//////////////////////////////////
///////////// METHODES //////////


/*	AF_INET			IPv4 Internet protocols
	SOCK_STREAM		TCP socket Provides sequenced, reliable, two-way, connection-based
				byte streams.
	SOCK_NONBLOCK	Set the O_NONBLOCK file status flag on  the  open  file
				description  (see  open(2)) referred to by the new file descriptor.
	O_NONBLOCK		When possible, the file is opened in nonblocking mode. Neither
				the  open()  nor  any  subsequent I/O operations on the file de‐
				scriptor which is returned will cause  the  calling  process  to
				wait.
sources : https://www.tutorialspoint.com/cplusplus/cpp_socket_programming.htm
*/
/**
 * Remarques >
 * 2. Le SOCK_NONBLOCK : Attention à la portabilité
 * Mettre SOCK_NONBLOCK directement dans socket() est une extension Linux (depuis le noyau 2.6.27).
 *  Le problème : À l'école 42, si tu corriges sur un Mac (souvent le cas en cluster), ça ne compilera pas ou ne marchera pas.
 * La solution "standard" : Créer la socket normalement, puis utiliser fcntl() pour la passer en non-bloquant. C'est plus verbeux, mais c'est ce qui est attendu.
 */
void	Server::init()
{
	// 1. Création du socket en IPv6 (qui fera aussi IPv4)
	int server_fd = socket(AF_INET6, SOCK_STREAM, 0);
	if (server_fd == -1)
		_exitWithError("socket failed");

	_serverSocket = server_fd;

	// 2. Configuration des options
		// A. Dual Stack : Accepter IPv4 et IPv6 sur ce socket
	int no = 0;
	if (-1 == setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)))
		_exitWithError("setsockopt IPv6only 'no' failed"); // Désactive "IPv6 only"

		// B. Réutiliser le port rapidement après un crash
	int opt = 1;
	if (-1 == setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) //pouvoir relancer le serveur immédiatement après l'avoir arrêté
		_exitWithError("setsockopt SO_REUSEADDR failed");

		// C. Non-bloquant (ATTENTION : desactive pour tester)
		// Si on le laisse actif sans utiliser poll(), accept() échouera tout de suite.
	if (-1 == fcntl(server_fd, F_SETFL, O_NONBLOCK))
		_exitWithError("Server fcntl(O_NONBLOCK) a échoué : ");

	////////////////////////////////////
	// 3. Préparation de l'adresse du SERVEUR (Pour le bind)
		// On utilise sockaddr_in6 car le socket est AF_INET6
	struct sockaddr_in6 address;
	int addrlen = sizeof(address);
	std::memset(&address, 0, addrlen);

	address.sin6_family = AF_INET6;
	address.sin6_addr = in6addr_any;// Équivalent IPv6 de INADDR_ANY pour ipv4
	address.sin6_port = htons(_port);

	// 4. Bind (Attacher le socket au port)
	if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0)
		_exitWithError("Bind failed");

	// 5. Listen
		//SOMAXCONN = Socket Maximum Connections
	if (listen(server_fd, SOMAXCONN) < 0)
		_exitWithError("Listen failed ");

	std::cout << "Serveur en attente sur le port " << _port << "..." << std::endl;
};				// socket, setsockopt, bind, listen

//https://beej.us/guide/bgnet/html/split-wide/slightly-advanced-techniques.html
/*
boucle attend qu'il se passe un event,
 si nouveau client → accept()
 si message client → recv()
recommencer
*/
void	Server::run()
{
	struct pollfd server_pfd;
	server_pfd.fd = _serverSocket;
	server_pfd.events = POLLIN;
	server_pfd.revents = 0;
	_fds.push_back(server_pfd);
	while (true && _Signal == false)
	{
		int ready = poll(_fds.data(), (nfds_t) _fds.size(), -1);
		if(-1 == ready) //https://beej.us/guide/bgnet/html/split-wide/slightly-advanced-techniques.html#:~:text=You%20can%20specify%20a%20negative%20timeout
		{
			if (_Signal)
				break;
			_exitWithError("Poll failed ");// exit or continue ??
		}

		// 3. On parcourt le tableau pour voir qui a généré un événement
		for (size_t i = 0; i < _fds.size(); i++) {
			if (_fds[i].revents == 0)
				continue;
			if (_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				client_disconnection(i);
				i--;
				continue;
			}
			if (_fds[i].revents & POLLIN)
			{
				if (_fds[i].fd == _serverSocket) //Event server = nouveau client
					acceptClient();
				else
				{
					if (false == receiveData(_fds[i].fd))
					{
						client_disconnection(i);
						i--;
						continue;
					}
					while (get_line_msg_to_cmd(_fds[i].fd))
					{
						parse_cmd(_fds[i].fd);
						exec_cmd(_fds[i].fd);
						_clients[_fds[i].fd]->clearCmd();
					}
				}
			}
		}

	}
	this->close_fds();
	std::cout << std::endl << "Signal received, server stopped" << std::endl;
};

void	Server::acceptClient()
{
	// 6. Préparation de la structure pour recevoir le CLIENT
	struct sockaddr_storage client_sa;
	socklen_t client_sa_len = sizeof(client_sa);
	int client_fd = accept(_serverSocket, (struct sockaddr *)&client_sa, &client_sa_len);
	if (client_fd < 0) {
		std::cerr << "Erreur accept : " << std::strerror(errno) << std::endl;
		return ;
	}

	if (-1 == fcntl(client_fd, F_SETFL, O_NONBLOCK)) {
		std::cerr << "Erreur fcntl client" << std::strerror(errno) << std::endl;
		close(client_fd);
		return;
	}

	if (_fds.size() > 1000)
	{
		std::cerr << "Serveur plein, rejet de la connexion du FD : " << client_fd << std::endl;
		std::string errMsg = "ERROR :Server is full\r\n";
		sendMessage(client_fd, "ERROR :Server is full.");
		close(client_fd);
		return;
	}

	//////write family-agnostic code, you should be using sockaddr_storage instead of sockaddr_in or sockaddr_in6 directly when possible. sockaddr_storage is large enough in size to hold both sockaddr_in and sockaddr_in6 structs.
	//https://stackoverflow.com/questions/13157151/isnt-struct-sockadr-in-supposed-to-work-for-both-ipv4-and-ipv6
	char client_ip[46];//max len for ipv6
	switch (client_sa.ss_family)
	{
		case AF_INET:
			if(NULL == inet_ntop(AF_INET, &(((sockaddr_in*)&client_sa)->sin_addr), client_ip, sizeof(client_ip)))
			{
				std::cerr << "inet_ntop AF_INET: " << std::strerror(errno) << std::endl;
				close(client_fd);
				return ;
			}
			std::cout << "Client connecté en IPv4: " << client_ip << std::endl;
			break;
		case AF_INET6:
			if(NULL == inet_ntop(AF_INET6, &(((sockaddr_in6*)&client_sa)->sin6_addr), client_ip, sizeof(client_ip)))
			{
				std::cerr << "inet_ntop: AF_INET6" << std::strerror(errno) << std::endl;
				close(client_fd);
				return ;
			}
			std::cout << "Client connecté en IPv6: " << client_ip << std::endl;
			break;
	}
	/////////////////////////////
	// 7 Assigner le noouveau client a la maap de clients
	_clients[client_fd] = new Client (client_fd, std::string(client_ip));

	// 8 rajouter  le fd du clients au polls de fds a surveiller
	struct pollfd pfd;
	pfd.fd = client_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_fds.push_back(pfd);

};


/*
	////EWOULDBLOCK : "Would Block" : Normalement, si on appelle recv() et qu'il n'y a rien, le programme s'arrête et attend (il "bloque"). Mais comme configuré en non-bloquant, le système refuse de s'arrêter.
	///continue la boucle, ne déconnecte pas.

*/
bool	Server::receiveData(int client_fd)
{
	std::cout << "Receiving data from fd :" << client_fd << std::endl;
	char buffer[1024] = {0};
	ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes_read > 0) {
		std::string msg(buffer, bytes_read);
		_clients[client_fd]->appendBuffer(msg);
		return true;
	}
	else if (bytes_read == 0) { // client a ferme la connection
		std::cout << "Client " << client_fd << " a fermé la connexion." << std::endl;
		return false;
	}
	else { // (-1) varif de si vide ou vrai erreur de recv()
		std::cerr << "recv() error on fd " << client_fd << std::endl;
		return false; // erreur réseau
	}
};	// recv() -> parsing



bool	Server::get_line_msg_to_cmd(int client_fd)
{
	std::string	&client_buff = _clients[client_fd]->getBuffer();
	size_t rn_position = client_buff.find("\n");// \r\n for irc
	if (rn_position == std::string::npos)
		return false;
	std::string cmd = client_buff.substr(0, rn_position);
	if (!cmd.empty() && cmd[cmd.size() - 1] == '\r')
		cmd.erase(cmd.size() - 1);
	_clients[client_fd]->setCmd(cmd);
	_clients[client_fd]->clearBuffer(0, rn_position + 1);
	return true;
}

void	Server::client_disconnection(size_t i)
{
	int fd = _fds[i].fd;

    std::map<int, Client*>::iterator itClient = _clients.find(fd);
    if (itClient == _clients.end())
    {
		return;
	}
	Client* client = itClient->second;

    if (!client)
		return;

	for (std::map<std::string, Channel*>::iterator it = _channels.begin();
         it != _channels.end();)
    {
        Channel* chan = it->second;
        chan->removeMember(client);
        chan->removeInvited(client);

        if (chan->isEmpty())
        {
            delete chan;
            _channels.erase(it++);
        }
        else
        {
            ++it;
        }
    }
    _clients.erase(itClient);
    delete client;

    close(fd);

    _fds.erase(_fds.begin() + i);

    std::cout << "Client fd " << fd << " disconnected\n";
}


void	Server::parse_cmd(int client_fd)
{
	std::string cmd = _clients[client_fd]->getCmd();
	cmd = trim(cmd);
}

void	Server::exec_cmd(int client_fd)
{
	std::string cmd = _clients[client_fd]->getCmd();
	std::cout << "[RAW CMD] " << cmd << std::endl;

	std::istringstream iss(cmd);
	std::string command;

	if (!(iss >> command))
    	return;

	if (command == "CAP") //pour connexion irssi, regle le pb de CAP LS
	{
		std::string sub;
    	iss >> sub;

		if (sub == "LS")
		{
			std::cout << "[CAP ACTION] sending LS response" << std::endl;//debug irssi
        	sendMessage(client_fd, "CAP * LS :");
		}
		else if (sub == "END")
    		return;
		return;
	}
	else if (command == "PASS")
		handlePass(client_fd, iss);
	else if (command == "NICK")
		handleNick(client_fd, iss);
	else if (command == "USER")
		handleUser(client_fd, iss);
	else if (command == "JOIN")
		handleJoin(client_fd, iss);
	else if (command == "KICK")
		handleKick(client_fd, iss);
	else if (command == "INVITE")
		handleInvite(client_fd, iss);
	else if (command == "PART")
		handlePart(client_fd, iss);
	else if (command == "PRIVMSG")
		handlePrivMsg(client_fd, iss);
	else if (command == "TOPIC")
		handleTopic(client_fd, iss);
	else if (command == "MODE")
		handleMode(client_fd, iss);
	else if (command == "PING")
		handlePing(client_fd, iss);
	else
		sendReply(client_fd, "421", command, "Unknown command");
}


//----VALIDATION NICKNAME----

bool Server::isValidNickname(std::string& nickname)
{
	if (nickname.empty() || nickname.size() > 9) //on se base sur RFC d'IRC
			return (false);
	if (!std::isalpha(nickname[0]) && nickname[0] != '_')
		return (false);

	for(size_t i = 0; i < nickname.size(); i++)
	{
		if(!std::isalnum(nickname[i]) && nickname[i] != '_') //a-z ou 0-9
			return (false);

		if (nickname[i] == '#' || nickname[i] == '&' || nickname[i] == ':')
			return (false);
	}
	return (true);
}

bool Server::nicknameUsed(std::string& nickname)
{
	std::string lowerNick = nickname;
	std::transform(lowerNick.begin(), lowerNick.end(), lowerNick.begin(), ::tolower); //convertit du debut a la fin et stocke dès le debut de la string

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		std::string existing = it->second->getNickname(); //commence a second (pointeur vers objet Client) car first est le fd du client
		std::transform(existing.begin(), existing.end(), existing.begin(), ::tolower);
		if (existing == lowerNick)
			return (true);
	}
	return (false);
}
