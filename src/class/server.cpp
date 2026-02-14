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

	address.sin6_family = AF_INET6;
	address.sin6_addr = in6addr_any;// Équivalent IPv6 de INADDR_ANY pour ipv4
	address.sin6_port = htons(_port);

	// 4. Bind (Attacher le socket au port)
	if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0)
		_exitWithError("Bind failed");

	// 5. Listen
	if (listen(server_fd, 3) < 0)
		_exitWithError("Listen failed ");

	std::cout << "Serveur en attente sur le port " << _port << "..." << std::endl;
};				// socket, setsockopt, bind, listen

void	Server::run()
{

	std::cout << "run() \n";
};				// Boucle while(true) avec poll()

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


	// 8. Lecture du message
	char buffer[1024] = {0};
	ssize_t bytes_read = read(client_fd, buffer, 1024 - 1);
	std::cout << "Message from client: " << buffer << std::endl;
	if (bytes_read > 0) {
		std::cout << "Message reçu : " << buffer << std::endl;
	}

};		// accept() -> new Client


void	Server::receiveData(int fd)
{

};	// recv() -> parsing

