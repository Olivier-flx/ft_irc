
#include "ft_irc.hpp"

static int	check_ac(int ac)
{
	if (ac <= 1 || ac > 3) {
		std::cout << "Err: invalid number of parameters" << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int	check_port_is_digit (char *port)
{
	int i = 0;
	while (port[i]) {
		if (isspace(port[i]))
			continue;
		if (port[i] < '0' || port[i] > '9'){
			std::cout << "Err: invalid <port> : contains non digit char(" << port[i] <<")" << std::endl;
			return EXIT_FAILURE;
		}
		i++;
	}
	return EXIT_SUCCESS;
}

// port is in range (0 - 65 535)
// https://en.cppreference.com/w/cpp/string/byte/strtol.html
static int	check_port_is_inrange(char *port, int  &port_int)
{
	// port is isdigit
	errno = 0;
	char* p_end = NULL;
	const long port_number = std::strtol(port, &p_end, 10);
	if (port == p_end) //aucun chiffre n'a été trouvé
		return EXIT_FAILURE;
	const bool range_error = errno == ERANGE;
	if (range_error) {
		std::cout << "Err: port Range error occurred.\n";
		return EXIT_FAILURE;
	}
	if (port_number < 0 || port_number > 65535) {
		std::cout << "Err: port must be between [0, 65 535].\n";
		return EXIT_FAILURE;
	}
	port_int = port_number;
	return EXIT_SUCCESS;
}

static int invalid_char_in_password(char *pw)
{
	int i = 0;
	while (pw[i])
	{
		if (isspace(pw[i])) {
			std::cout << "Err: invalid password : space not allowed" << std::endl;
			return EXIT_FAILURE;
		}
		if (pw[i] == '\'' || pw[i] == ';' || pw[i] == '\\' || pw[i] == '`' || pw[i] == '"') {
			std::cout << "Err: invalid password : unauthorised character in password ('\'', ';', '\\', '`' or '\"')" << std::endl;
			return EXIT_FAILURE;
		}
		if (pw[i] < 21 || pw[i] > 126){
			std::cout << "Err: invalid password : unvalid character in password" << std::endl;
			return EXIT_FAILURE;
		}
		i++;
	}
	return EXIT_SUCCESS;
}

bool	arg_ok(int ac, char **argv, int &port_int){
	int err = 0;

	err += check_ac(ac);
	if (err > 0) return false;
	err += check_port_is_digit(argv[1]);
	if (err > 0) return false;
	err += check_port_is_inrange(argv[1], port_int);
	if (err > 0) return false;
	err += invalid_char_in_password(argv[2]);
	if (err != 0)
		return false;
	return (true);
}
