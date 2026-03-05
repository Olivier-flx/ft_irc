#include "ft_irc.hpp"
#include "Server.hpp"

/*
std::istringstream transforme string en flux de lecture
on peut utiliser >> pour lire mot par mot 
(séparé par les espaces)
*/

void Server::handlePass(int fd, std::istringstream &iss)
{
    std::string pass;
    iss >> pass;

    if (pass.empty())
    {
        sendMessage(fd, "461 PASS: Not enough parameters");
        return;
    }

	std::string extra;
    if (iss >> extra) // s'il y a un autre mot après pass
	{
        sendMessage(fd, "461 PASS: Too many parameters");
        return;	
    }

    if (pass != _password)
    {
        sendMessage(fd, "464 PASS: Password incorrect");
        return;
    }
    _clients[fd]->setAuthenticated(true);
    tryRegister(fd);
}

/*
Dans vrai IRC le client analyse le code numerique en 
debut de message (ex 432 : no nickename),
dans notre cas pas obligatoire pour nc, mais
c'est RFC-compliant etçca fait plus réaliste 
*/

void Server::handleNick(int fd, std::istringstream &iss)
{
    std::string nickname;
    iss >> nickname;

    if (nickname.empty())
    {
        sendMessage(fd, "431 :No nickname given");
        return;
    }

    if (!isValidNickname(nickname))
    {
        sendMessage(fd, "432 " + nickname + " :Erroneous nickname");
        return;
    }

    if (nicknameUsed(nickname))
    {
        sendMessage(fd, "433 " + nickname + " :Nickname is already in use");
        return;
    }
    _clients[fd]->setNickname(nickname);
    tryRegister(fd);
}

void Server::handleUser(int fd, std::istringstream &iss)
{
    std::string username;
    iss >> username;

    if (username.empty())
    {
        sendMessage(fd, "461 USER :Not enough parameters");
        return;
    }
    _clients[fd]->setUsername(username);
    tryRegister(fd);
}

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
        sendMessage(fd, ":ft_irc 003 " + client->getNickname() + " :This server was created " + getCreationTime());
        sendMessage(fd, ":ft_irc 004 " + client->getNickname() + " ft_irc 0.1 -i -t -k -l -o"); //modes dispos pour les channels
    }
}


void Server::sendMessage(int fd, const std::string &msg)
{
    std::string full = msg + "\r\n";  // IRC = CRLF obligatoire, sinon irc bug car il attend fin du msg
    send(fd, full.c_str(), full.size(), 0); //fonction en c, donc on doit cast la string
}
	

void Server::handleJoin(int fd, std::istringstream &iss)
{
    std::string channelName;
    iss >> channelName; //recuperation du 1er arg après JOIN

    if (channelName.empty())
    {
        sendMessage(fd, "461 JOIN :Not enough parameters");
        return;
    }

    if (_channels.find(channelName) == _channels.end()) // vérifie si le channel existe déja, sinon le crée
        _channels[channelName] = new Channel(channelName);

    Channel* ch = _channels[channelName]; //on garde le channel
    Client* client = _clients[fd]; //et le client qui fait JOIN

    std::vector<Client*> members = ch->getMembers();

    if (std::find(members.begin(), members.end(), client) != members.end())
    {
        sendMessage(fd, "443 " + client->getNickname() + " " + channelName + " :is already on channel");
        return;
    }
  
    ch->addMember(client);
    
    members = ch->getMembers();
    std::string join_msg = ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
    for (std::vector<Client*>::iterator it = members.begin(); it != members.end(); ++it)
    {   
        Client* m = *it;
        int ret = send(m->getFd(), join_msg.c_str(), join_msg.size(), 0); //broadcast le join à chaque membre du channel
        if (ret == -1)
            std::cerr << "ERROR failed sending JOIN to " << m->getNickname() << std::endl;
    }

    if (!ch->getTopic().empty())
    {
        std::string topic_msg = ":ft_irc 332 " + client->getNickname() + " " + channelName + " :" + ch->getTopic();
        sendMessage(fd, topic_msg); //msg envoyé uniquement à celui qui rejoint
    }
}


void Server::handlePrivMsg(int fd, std::istringstream &iss)
{
    std::string target;
    iss >> target; //channel ou nickname 

    if (target.empty())
    {
        sendMessage(fd, "411 :No recipient given (PRIVMSG)");
        return;
    }

    std::string message;
    std::getline(iss, message);

    if (!message.empty() && message[0] == ':')
        message.erase(0, 1);

    if (message.empty())
    {
        sendMessage(fd, "412 :No text to send");
        return;
    }

    Client* sender = _clients[fd];

    if (target[0] == '#' || target[0] == '&') 
    {
        if (_channels.find(target) == _channels.end())
        {
            sendMessage(fd, "403 " + target + " :No such channel");
            return;
        }

        Channel* ch = _channels[target];

        std::vector<Client*> members = ch->getMembers();
        if (std::find(members.begin(), members.end(), sender) == members.end())
        {
            sendMessage(fd, "404 " + target + " :Cannot send to channel (not member)");
            return;
        }

        std::string full_msg = ":" + sender->getHostname() + " PRIVMSG " + target + " :" + message + "\r\n";

        members = ch->getMembers();
        for (std::vector<Client*>::iterator it = members.begin(); it != members.end(); ++it) // envoi à tous les membres sauf à celui qui envoie le msg
        {
            Client* c = *it;
            if (c->getFd() != fd)
                send(c->getFd(), full_msg.c_str(), full_msg.size(), 0);
        }
    }
    else
    {
        Client* receiver = nullptr;
        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            if (it->second->getNickname() == target)
            {
                receiver = it->second;
                break;
            }
        }

        if (!receiver)
        {
            sendMessage(fd, "401 " + target + " :No such nick");
            return;
        }

        std::string full_msg = ":" + sender->getHostname() + " PRIVMSG " + target + " :" + message + "\r\n";
        send(receiver->getFd(), full_msg.c_str(), full_msg.size(), 0);
    }
}