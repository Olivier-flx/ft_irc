
#include "ft_irc.hpp"
#include "class.hpp"

/**
 * Pour tester : Ouvrir deux terminaux
 * Dans le premier lancer le programme.
 *
 * Dans le second, `nc 127.0.0.1 8080`  ou `nc -6 ::1 8080`
 * puis taper le message "Hello" puis la touche enter
 *
 * puis la connextion se ferme
 * Prochaines etapes
 * 		--> Implementer create_serverSocket dans une boucle
 * 		--> utiliser les class client et serveur
 *
 */


int	main(int ac, char **argv)
{
	int	port = 0;
	if (!arg_ok(ac, argv, port))
		return (EXIT_FAILURE);
	std::printf("port: %i\n", port);
	std::string password = argv[2];

	try
	{
		Server serv(port, password);
		std::signal(SIGINT, Server::SignalHandler); //gère signal (ctrl + c)
		std::signal(SIGQUIT, Server::SignalHandler); //-> gère signal (ctrl + \)
		serv.init();
		serv.run();
	}
	catch(const std::exception& e)
	{
		std::cerr << "Err:" << e.what() << '\n';
		return (EXIT_FAILURE);
	}

	// create socket
	// Bind the socket to a IP / port
	// Mark the socket for listening in
	// accept a call
	// close the listening socket
	// while reveiving display message , echo message
	// close socket

	return EXIT_SUCCESS;
}
