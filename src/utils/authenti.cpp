#include "ft_irc.hpp"

void handlePass(int fd, std::istringstream &iss);
		void handleNick(int fd, std::istringstream &iss);
		void handleUser(int fd, std::istringstream &iss);


void Server::tryRegister(int fd)
{
    Client *client = _clients[fd];

    if (client->isRegistered())
        return;

    if (client->isAuthenticated() && !client->getNickname().empty() && !client->getUsername().empty())
    {
        client->setRegistered(true);

        sendMessage(fd, ":ft_irc 001 " + client->getNickname() + " :Welcome to ft_irc Network");
        sendMessage(fd, ":ft_irc 002 " + client->getNickname() + " :Your host is ft_irc");
        sendMessage(fd, ":ft_irc 003 " + client->getNickname() + " :This server was created " + serv.getCreationTime());
        sendMessage(fd, ":ft_irc 004 " + client->getNickname() + " ft_irc 0.1 -i -t -k -l -o"); //modes dispos pour les channels
    }
}


void Server::sendMessage(int fd, const std::string &msg)
{
    std::string full = msg + "\r\n";  // IRC = CRLF obligatoire
    send(fd, full.c_str(), full.size(), 0);
}
	