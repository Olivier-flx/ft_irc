#include "ft_irc.hpp"

// trim from end of string (right)
std::string& rtrim(std::string& s)
{
	const char* ws = " \t\n\r\f\v";
	size_t end = s.find_last_not_of(ws);

	if (end != std::string::npos)
		s.erase(end + 1);
	else
		s.clear(); // toute la chaîne était des espaces
	return (s);
}

// trim from beginning of string (left)
std::string& ltrim(std::string& s)
{
	const char* ws = " \t\n\r\f\v";
	size_t start = s.find_first_not_of(ws);

	if (start != std::string::npos)
		s.erase(0, start);
	else
		s.clear();
	return (s);
}

// trim from both ends of string (right then left)
std::string&	trim(std::string& str)
{
	return ltrim(rtrim(str));
}
