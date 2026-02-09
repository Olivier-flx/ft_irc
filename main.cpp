
#include "ft_irc.hpp"

int	main(int ac, char **argv)
{
	std::vector<std::string> arguments;

	if (ac > 1)
		arguments.assign(argv + 1, argv + ac);
	if (!arg_ok(ac, argv))
		return (EXIT_FAILURE);
	//create socket
	//Bind the socket to a IP / port
	// Mark the socket for listening in
	// accept a call
	// close the listening socket
	// while reveiving display message , echo message
	//close socket

	return EXIT_SUCCESS;
}
