#include "ft_irc.hpp"
#include "Server.hpp"

/*
std::istringstream transforme string en flux de lecture
on peut utiliser >> pour lire mot par mot
(séparé par les espaces)
*/

//commandes d'authentification

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
    if (!_clients[fd]->isAuthenticated()) //car le bon mdp doit être renseigné en 1er
    {
        sendMessage(fd, "464 :Password required");
        return;
    }

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
    if (!_clients[fd]->isAuthenticated())
    {
        sendMessage(fd, "464 :Password required");
        return;
    }
    
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

		sendMessage(fd, ":" + _serverName + " 001 " + client->getNickname() + " :Welcome to " +_serverName + " Network");
		sendMessage(fd, ":" + _serverName + " 002 " + client->getNickname() + " :Your host is " +_serverName);
		sendMessage(fd, ":" + _serverName + " 003 " + client->getNickname() + " :This server was created " + getCreationTime());
		sendMessage(fd, ":" + _serverName + " 004 " + client->getNickname() + " " + _serverName + " 0.1 -i -t -k -l -o"); //modes dispos pour les channels
	}
}


void Server::sendMessage(int fd, const std::string &msg)
{
	std::string full = msg + "\r\n";  // IRC = CRLF obligatoire, sinon irc bug car il attend fin du msg
	send(fd, full.c_str(), full.size(), 0); //fonction en c, donc on doit cast la string
}

//commandes channel

bool	Server::cannotJoinChannel(Channel *ch, Client *client, const std::string &key)
{
	// Vérification mode +i (invite only)
	if (ch->isInviteOnly() && !ch->isInvited(client))
	{
		sendMessage(client->getFd(), "473 " + ch->getName() + " :Cannot join channel (+i)");
		return (true);
	}
	// Vérification mode +k (clé)
	if (!ch->getKey().empty() && key != ch->getKey())
	{
		sendMessage(client->getFd(), "475 " + ch->getName() + " :Cannot join channel (+k)");
		return (true);
	}
	// Vérification mode +l (limite)
	if (ch->getLimit() > 0 && (int) ch->getMembers().size()  >= ch->getLimit())
	{
		sendMessage(client->getFd(), "471 " + ch->getName() + " :Cannot join channel (+l)");
		return (true);
	}
	return (false);
}

void Server::handleJoin(int fd, std::istringstream &iss)
{
	std::string channelName;
	std::string key;
	iss >> channelName;  //recuperation du 1er arg après JOIN
	iss >> key; // recuperation du mot de passe

	std::cout << "DEBUG : channel joined name = '" + channelName + "'" << std::endl;
	if (channelName.empty())
	{
		sendMessage(fd, "461 JOIN :Not enough parameters");
		return;
	}
	if (channelName[0] != '#' && channelName[0] != '&')
	{
		sendMessage(fd, "476 " + channelName + " :Bad channel mask");
		return;
	}

	if (_channels.find(channelName) == _channels.end()) // vérifie si le channel existe déja, sinon le crée
		_channels[channelName] = new Channel(channelName);

	Channel* ch = _channels[channelName]; //on recup l'obj channel
	Client* client = _clients[fd]; //et le client qui fait JOIN

	std::vector<Client*> members = ch->getMembers();
	if (std::find(members.begin(), members.end(), client) != members.end())
	{
		sendMessage(fd, "443 " + client->getNickname() + " " + channelName + " :is already on channel");
		return;
	}
	if (cannotJoinChannel(ch, client, key))
		return;

	ch->addMember(client);
	ch->removeInvited(client);

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
		std::string topic_msg = ":" + _serverName + " 332 " + client->getNickname() + " " + channelName + " :" + ch->getTopic();
		sendMessage(fd, topic_msg); //msg envoyé uniquement à celui qui rejoint
	}
	std::cout << "DEBUG: inviteOnly=" << ch->isInviteOnly()
          << " invited=" << ch->isInvited(client)
          << std::endl;

	// TODO
	// RPL_NAMREPLY 353 + 366
	//353 ton_nick = #channel :@admin1 user2 user3
	//366 ton_nick #channel :End of /NAMES list
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

	if (message.empty())
	{
		sendMessage(fd, "412 :No text to send");
		return;
	}

	if (message[0] == ':')
		message.erase(0, 1);

	Client* sender = _clients[fd];

	if (target[0] == '#' || target[0] == '&')
	{
		if (_channels.find(target) == _channels.end())
		{
			sendMessage(fd, "403 " + target + " :No such channel");
			return;
		}

		Channel* ch = _channels[target];

		const std::vector<Client*>& members = ch->getMembers();
		if (std::find(members.begin(), members.end(), sender) == members.end())
		{
			sendMessage(fd, "404 " + target + " :Cannot send to channel (not member)");
			return;
		}

		std::string full_msg = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname() + " PRIVMSG " + target + " :" + message + "\r\n";
		for (std::vector<Client*>::const_iterator it = members.begin(); it != members.end(); ++it) // envoi à tous les membres sauf à celui qui envoie le msg
		{
			Client* c = *it;
			if (c != sender)
				send(c->getFd(), full_msg.c_str(), full_msg.size(), 0);
		}
	}
	else
	{
		Client* receiver = NULL;
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

		std::string full_msg = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname() + " PRIVMSG " + target + " :" + message + "\r\n";
		send(receiver->getFd(), full_msg.c_str(), full_msg.size(), 0);
	}
}

void Server::handleTopic(int fd, std::istringstream &iss)
{
	std::string channelName;
	iss >> channelName;

	if (channelName.empty())
	{
		sendMessage(fd, "461 TOPIC :You must specify a channel name");
		return;
	}

	if (_channels.find(channelName) == _channels.end())
	{
		sendMessage(fd, "403 " + channelName + " :No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd];

	const std::vector<Client*>& members = ch->getMembers();

	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendMessage(fd, "442 " + channelName + " :You're not on that channel");
		return;
	}

	std::string topic;
	std::getline(iss, topic);

	if (!topic.empty() && topic[0] == ':')
		topic.erase(0, 1);

	if (topic.empty())
	{
		if (ch->getTopic().empty())
			sendMessage(fd, "331 " + client->getNickname() + " " + channelName + " :No topic set");
		else
			sendMessage(fd, "332 " + client->getNickname() + " " + channelName + " :" + ch->getTopic());
		return;
	}

	if (ch->isTopicRestricted() && !ch->isAdmin(client)  && !client->isOperator()) //modif du topic
	{
		sendMessage(fd, "482 " + channelName + " :You're not channel operator");
		return;
	}

	ch->setTopic(topic);

	std::string msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " TOPIC " + channelName + " :" + topic + "\r\n";;

	for (std::vector<Client*>::const_iterator it = ch->getMembers().begin(); it != ch->getMembers().end(); ++it)
	{
		Client* c = *it;
		send(c->getFd(), msg.c_str(), msg.size(), 0);
	}
}



void Server::handlePart(int fd, std::istringstream &iss)
{
	std::string channelName;
	iss >> channelName;

	if (channelName.empty())
	{
		sendMessage(fd, "461 PART :You must specify a channel name");
		return;
	}

	if (_channels.find(channelName) == _channels.end())
	{
		sendMessage(fd, "403 " + channelName + " :No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd];

	const std::vector<Client*>& members = ch->getMembers();
	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendMessage(fd, "442 " + channelName + " :You're not on that channel");
		return;
	}

	std::string msg;
	std::getline(iss, msg);
	if (!msg.empty() && msg[0] == ':')
		msg.erase(0, 1);

	// prévenir tous les membres
	std::string notify = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " PART " + channelName;
	if (!msg.empty()) notify += " :" + msg;
	notify += "\r\n";

	for (std::vector<Client*>::const_iterator it = ch->getMembers().begin(); it != ch->getMembers().end(); ++it)
	{
		Client* c = *it;
		send(c->getFd(), notify.c_str(), notify.size(), 0);
	}

	ch->removeMember(client);

	if (ch->getMembers().empty())
		_channels.erase(channelName);
}


Client* Server::getClientByNick(const std::string& nick)
{
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == nick)
			return it->second;
	}
	return (NULL); // pas trouvé
}


void Server::handleInvite(int fd, std::istringstream &iss)
{
	std::string nick, channelName;
	iss >> nick >> channelName; //stocke 1er arg dans nick et mot suivant dans channelName

	if (nick.empty() || channelName.empty())
	{
		sendMessage(fd, "461 INVITE :Not enough parameters, you must specify a nick and a channel");
		return;
	}

	if (_channels.find(channelName) == _channels.end())
	{
		sendMessage(fd, "403 " + channelName + " :No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd];

	const std::vector<Client*>& members = ch->getMembers(); //verifie si celui qui demande fait bien partie du channel
	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendMessage(fd, "442 " + channelName + " :You're not on that channel");
		return;
	}

	if (ch->isInviteOnly() && !ch->isAdmin(client)) // si channel est +i, seuls operators peuvent inviter
	{
		sendMessage(fd, "482 " + channelName + " :You're not channel operator");
		return;
	}

	Client* target = getClientByNick(nick); //cherche la cible dans le serveur
	if (!target)
	{
		sendMessage(fd, "401 " + nick + " :No such nick");
		return;
	}

	if (std::find(members.begin(), members.end(), target) != members.end()) //verifie si pas deja dans le channel
	{
		sendMessage(fd, "443 " + nick + " " + channelName + " :User already on channel");
		return;
	}

	ch->addInvited(target);

	std::string prefix = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();

	sendMessage(target->getFd(), prefix + " INVITE " + nick + " " + channelName); //envoi du msg d'invit à l'invité

	sendMessage(fd, "341 " + client->getNickname() + " " + nick + " " + channelName); //confirmation d'envoi au client qui a fait la demande
}

void Server::handleMode(int fd, std::istringstream &iss)
{
	std::string channelName;
	iss >> channelName;

	if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
		return; //mode utilisateur a la connection, donc ignore

	if (_channels.find(channelName) == _channels.end())
	{
		sendMessage(fd, "403 " + channelName + " :No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd];

	const std::vector<Client*>& members = ch->getMembers();
	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendMessage(fd, "442 " + channelName + " :You're not on that channel");
		return;
	}

	if (!ch->isAdmin(client) && !client->isOperator())
	{
		sendMessage(fd, "482 " + channelName + " :You're not channel operator");
		return;
	}

	std::string mode;
	iss >> mode;

	std::vector<std::string> params;
	std::string tmp;

	while (iss >> tmp)
		params.push_back(tmp);

	size_t paramIndex = 0;

	if (mode.empty())
	{
		sendMessage(fd, "324 " + channelName + " :" + ch->getModes());
		return;
	}

	bool add = true;
	std::string modeStr; // pour recuperer les lettres et les signes + ou -
	std::vector<std::string> paramValue; // pour recuperer les parametres de K, o, ou l

	for (size_t i = 0; i < mode.size(); i++)
	{
		char m = mode[i];

		if (m == '+')
			add = true, modeStr += "+";
		else if (m == '-')
			add = false, modeStr += "-";
		else if (m == 't')
			ch->setTopicRestriction(add), modeStr += "t";
		else if (m == 'i')
		{
			ch->setInviteOnly(add);
			modeStr += "i";
			std::cout << "MODE +i set to " << add << std::endl; //debug
		}
		else if (m == 'o') // donner/retirer droits opérateur
		{
			if (paramIndex >= params.size())
			{
				sendMessage(fd, "461 MODE :Not enough parameters");
				continue;
			}
			std::string targetNick = params[paramIndex++];

			Client* target = getClientByNick(targetNick);
			if (!target || std::find(members.begin(), members.end(), target) == members.end())
			{
				sendMessage(fd, "441 " + targetNick + " " + channelName + " :They aren't on that channel");
				continue;
			}
			if (add)
				ch->addAdmin(target);
			else
			{
				if (ch->getMembers().size() == 1) // seul membre = seul admin
				{
					sendMessage(fd, ":" + _serverName + " NOTICE " + client->getNickname()
					+ " :Cannot remove admin rights: you are the only member of " + channelName);
					continue;
				}
				if (ch->getAdmins().size() == 1 && ch->isAdmin(target)) // droit d'admin transmit au premier membre
				{
					//  transfere de droits a un membre
					ch->removeAdmin(target); // promotion automatique faite dans removeAdmin()
					// Notifier le nouveau admin
					Client* newAdmin = ch->getAdmins()[0];
					sendMessage(newAdmin->getFd(), ":" + _serverName + " NOTICE " + newAdmin->getNickname()
						+ " :You have been promoted to channel operator of " + channelName);
				}
				else
					ch->removeAdmin(target);
			}
			modeStr += "o";
			paramValue.push_back(targetNick);
		}
		else if (m == 'k') // mot de passe channel
		{
			std::string key;

			if (add)
			{
				if (paramIndex >= params.size())
				{
					sendMessage(fd, "461 MODE :Not enough parameters");
					continue;
				}
				key = params[paramIndex++];
			}
			else
				key = "";

			ch->setMode('k', key);

			if (!key.empty())
				paramValue.push_back(key);

			modeStr += "k";
		}
		else if (m == 'l') // limite de clients
		{
			std::string limit;

			if (add)
			{
				if (paramIndex >= params.size())
				{
					sendMessage(fd, "461 MODE :Not enough parameters");
					continue;
				}
				limit = params[paramIndex++];
			}
			else
				limit = "";

			ch->setMode('l', limit);

			if (!limit.empty())
				paramValue.push_back(limit);

			modeStr += "l";
		}
		else
			sendMessage(fd, "472 " + std::string(1, m) + " :Unknown mode char");
	}

	std::string msg = ":" + client->getNickname()
					+ "!" + client->getUsername()
					+ "@" + client->getHostname()
					+ " MODE " + channelName
					+ " " + modeStr;
	for (size_t i = 0; i < paramValue.size(); i++)
		msg += " " + paramValue[i];
	msg += "\r\n";

	for (std::vector<Client*>::const_iterator it = ch->getMembers().begin();
			it != ch->getMembers().end(); ++it) //envoi à tous les membres
		send((*it)->getFd(), msg.c_str(), msg.size(), 0);
}


void Server::handleKick(int fd, std::istringstream &iss)
{
	std::string channelName, targetNick;
	iss >> channelName >> targetNick;

	if (channelName.empty() || targetNick.empty())
	{
		sendMessage(fd, "461 KICK :Not enough parameters (must specify channel and user)");
		return;
	}

	if (_channels.find(channelName) == _channels.end())
	{
		sendMessage(fd, "403 " + channelName + " :No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd]; // client qui fait la demande

	const std::vector<Client*>& members = ch->getMembers();
	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendMessage(fd, "442 " + channelName + " :You're not on that channel");
		return;
	}

	if (!ch->isAdmin(client) && !client->isOperator())
	{
		sendMessage(fd, "482 " + channelName + " :You're not channel operator");
		return;
	}

	Client* target = getClientByNick(targetNick);  // récupérer le client à kicker
	if (!target || std::find(members.begin(), members.end(), target) == members.end())
	{
		sendMessage(fd, "441 " + targetNick + " " + channelName + " :They aren't on that channel");
		return;
	}

	std::string reason;  //pour message de kick
	std::getline(iss, reason);
	if (!reason.empty() && reason[0] == ':')
		reason.erase(0, 1);

	std::string kick_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() +
						   " KICK " + channelName + " " + targetNick;
	if (!reason.empty())
		kick_msg += " :" + reason;
	kick_msg += "\r\n";

	for (std::vector<Client*>::const_iterator it = members.begin(); it != members.end(); ++it) //info à tous les membres
	{
		Client* c = *it;
		send(c->getFd(), kick_msg.c_str(), kick_msg.size(), 0);
	}

	ch->removeMember(target);

	if (ch->getMembers().empty()) //supp du channel si vide
		_channels.erase(channelName);
}

void Server::handlePing(int fd, std::istringstream &iss)
{
	std::string token;
	iss >> token;
	if (token[0] == ':')
		token.erase(0, 1);
	sendMessage(fd, ":" + _serverName + " PONG " + _serverName + " :" + token);
	std::cout << ":" + _serverName + " PONG " + _serverName + " :" + token << std::endl;  // DEBUG
}
