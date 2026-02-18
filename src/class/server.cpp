#include "ft_irc.hpp"
#include "Server.hpp"

Server::Server( void ){
	std::cout << "Server : Default Constructor called" << std::endl;
};

Server::Server(std::string password, int port)
				:	_password (password),
					_port(port)
{
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
		//_channels = src._channels;
		_password = src._password;
	}
	return (*this);
};

Server::~Server() {
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
	std::cout << "Server : Destructor called" << std::endl;
};


void Server::_exitWithError(const std::string& msg)
{
	if (_serverSocket != -1) {
		close(_serverSocket);
		_serverSocket = -1;
	}
	throw std::runtime_error(msg + ": " + std::strerror(errno));
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
void	Server::run()
{
	struct pollfd server_pfd;
	server_pfd.fd = _serverSocket;
	server_pfd.events = POLLIN;
	server_pfd.revents = 0;
	_fds.push_back(server_pfd);
	while (true)
	{
		if(-1 == poll(_fds.data(), (nfds_t) _fds.size(), -1)) //https://beej.us/guide/bgnet/html/split-wide/slightly-advanced-techniques.html#:~:text=You%20can%20specify%20a%20negative%20timeout
				_exitWithError("Poll failed ");
		// 3. On parcourt le tableau pour voir qui a généré un événement
		for (size_t i = 0; i < _fds.size(); i++) {
			if (_fds[i].revents == 0)
				continue;
			if (_fds[i].revents & POLLIN)
			{
				if (_fds[i].fd == _serverSocket) //Event server = nouveau client
					this->acceptClient();
				else
					this->receiveData(_fds[i].fd);
			}
		}

	}
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


void	Server::receiveData(int client_fd)
{
	std::cout << "Receiving data from fd :" << client_fd << std::endl;
	char buffer[1024] = {0};
	ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes_read > 0) {
		std::cout << "Message reçu : " << buffer << std::endl;
	}
	std::string msg = (buffer);
	_clients[client_fd]->set_buffer(msg);
	parsing_msg(client_fd, msg);
};	// recv() -> parsing

/**
 * KICK - Eject a client from the channel
 * 		format : KICK <channel> <user> [<comment>]
 * INVITE - Invite a client to a channel
 * 		format : INVITE <nickname> <channel>
 * TOPIC - Change or view the channel topic
 * 		format : TOPIC <channel> [<topic>]
 * MODE - Change the channel’s mode:
 * 		· i: Set/remove Invite-only channel
 * 		· t: Set/remove the restrictions of the TOPIC command to channel operators
 * 		· k: Set/remove the channel key (password)
 * 		· o: Give/take channel operator privilege
 * 		· l: Set/remove the user limit to channel
 * 		format : MODE <channel> <modes> [<params>]
 * 			exemples : MODE #channel +it
 * 						MODE #42 -t
 * 						MODE #42 +k secret123
 */

void	remove_trailing_rn(std::string &msg)
{
	//int len = msg.length();
	msg.erase(msg.end()-2, msg.end());
	std::cout << "Msg without trailing elements :`" << msg <<"`\n";
}

void	Server::parsing_msg(int client_fd, std::string msg)
{
	std::string	client_buff = _clients[client_fd]->get_buffer();
	size_t rn_position = client_buff.find("\r\n");
	if (rn_position == std::string::npos)
		return ;
	std::string cmd = client_buff.erase(rn_position, client_buff.length());


	(void)client_fd;

	remove_trailing_rn(msg);

}
