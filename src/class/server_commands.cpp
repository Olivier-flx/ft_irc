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
	if (_clients[fd]->isRegistered())
	{
		sendReply(fd, "462", "", "Already registered");
		return;
	}

	std::string pass;
	iss >> pass;

	if (pass.empty())
	{
		sendReply(fd, "461", "PASS", "Not enough parameters");
		return;
	}

	std::string extra;
	if (iss >> extra) // s'il y a un autre mot après pass
	{
		sendReply(fd, "461", "PASS", "Too many parameters");
		return;
	}

	if (pass != _password)
	{
		sendReply(fd, "464", "", "Password incorrect");
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
		sendReply(fd, "464", "", "Password required");
		return;
	}
	std::string nickname;
	if (!(iss >> nickname))
	{
		sendReply(fd, "431", "", "No nickname given");
		return;
	}
	if (nickname.find(' ') != std::string::npos || !isValidNickname(nickname))//npos signifie introuvable
	{
		sendReply(fd, "432", nickname, "Erroneous nickname");
		return;
	}
	if (nicknameUsed(nickname))
	{
		sendReply(fd, "433", nickname, "Nickname is already in use");
		return;
	}
	_clients[fd]->setNickname(nickname);
	tryRegister(fd);
}

void Server::handleUser(int fd, std::istringstream &iss)
{
	if (_clients[fd]->isRegistered())
	{
		sendReply(fd, "462", "", "Already registered");
		return;
	}

	if (!_clients[fd]->isAuthenticated())
	{
		sendReply(fd, "464", "", "Password required");
		return;
	}

	std::string username;
	iss >> username;

	if (username.empty())
	{
		sendReply(fd, "461", "USER", "Not enough parameters");
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

void Server::sendReply(int fd, const std::string& code, const std::string& args, const std::string& msg)
{
	std::string target = _clients[fd]->getNickname();
	if (target.empty())
		target = "*";

	std::string full_msg = ":" + _serverName + " " + code + " " + target;

	// ex: le pseudo rejeté
	if (!args.empty())
		full_msg += " " + args;

	if (!msg.empty())
		full_msg += " :" + msg;

	sendMessage(fd, full_msg);
}

void Server::sendMessage(int fd, const std::string &msg)
{
	std::string full = msg + "\r\n";  // IRC = CRLF obligatoire, sinon irc bug car il attend fin du msg
	send(fd, full.c_str(), full.size(), 0); //fonction en c, donc on doit cast la string
}

//commandes channel

bool	Server::cannotJoinChannel(Channel *ch, Client *client, const std::string &key)
{
	int fd = client->getFd();

	// Vérification mode +i (invite only)
	if (ch->isInviteOnly() && !ch->isInvited(client))
	{
		sendReply(fd, "473", ch->getName(), "Cannot join channel (+i)");
		return (true);
	}
	// Vérification mode +k (clé)
	if (!ch->getKey().empty() && key != ch->getKey())
	{
		sendReply(fd, "475", ch->getName(), "Cannot join channel (+k)");
		return (true);
	}
	// Vérification mode +l (limite)
	if (ch->getLimit() > 0 && (int) ch->getMembers().size()  >= ch->getLimit())
	{
		sendReply(fd, "471", ch->getName(), "Cannot join channel (+l)");
		return (true);
	}
	return (false);
}

void Server::handleJoin(int fd, std::istringstream &iss)
{
	std::cout << "DEBUG: On arrive dans handlejoin" << std::endl; //debug efface
	Client* client = _clients[fd];//client qui fait join

	if (!client->isRegistered()|| client->getNickname().empty() || client->getUsername().empty())
		return;

	std::string channelName;
	std::string key;

	if (!(iss >> channelName))//recuperation du 1er arg après JOIN
	{
		sendReply(fd, "461", "JOIN", "Not enough parameters");
		return;
	}

	iss >> key; // recuperation du mot de passe

	if (channelName.empty() || channelName == ":"  || channelName[0] == ':')
	{
		sendReply(fd, "461", "JOIN", "Not enough parameters");
		return;
	}

	if (channelName.size() < 2 || (channelName[0] != '#' && channelName[0] != '&'))
	{
		sendReply(fd, "476", channelName, "Bad channel mask");
		return;
	}

	if (_channels.find(channelName) == _channels.end()) // vérifie si le channel existe déja, sinon le crée
		_channels[channelName] = new Channel(channelName);

	Channel* ch = _channels[channelName]; //on recup l'obj channel

	std::vector<Client*> members = ch->getMembers();
	if (std::find(members.begin(), members.end(), client) != members.end())
	{
		sendReply(fd, "443", client->getNickname() + " " + channelName, "is already on channel");
		return;
	}
	if (cannotJoinChannel(ch, client, key))
		return;

	ch->addMember(client);
	ch->removeInvited(client);

	const std::vector<Client*>& newMembers = ch->getMembers();//// on reprend les membres APRÈS modification

	std::string join_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " JOIN " + channelName;

	for (std::vector<Client*>::const_iterator it = newMembers.begin(); it != newMembers.end(); ++it)
		sendMessage((*it)->getFd(), join_msg);

	// 332 RPL_TOPIC
	if (!ch->getTopic().empty())
		sendReply(fd, "332", channelName, ch->getTopic());

	std::cout << "DEBUG: inviteOnly=" << ch->isInviteOnly()
			<< " invited=" << ch->isInvited(client) << std::endl;

	// RPL_NAMREPLY 353 : liste des membres
	std::string namesList = "";
	const std::vector<Client*>& updatedMembers = ch->getMembers();
	for (std::vector<Client*>::const_iterator it = updatedMembers.begin(); it != updatedMembers.end(); ++it)
	{
		if (ch->isAdmin(*it))
			namesList += "@";
		namesList += (*it)->getNickname();
		if (it + 1 != updatedMembers.end())
			namesList += " ";
	}
	sendReply(fd, "353", "= " + channelName, namesList);

	// RPL_ENDOFNAMES 366
	sendReply(fd, "366", channelName, "End of /NAMES list");
}


void Server::handlePrivMsg(int fd, std::istringstream &iss)
{
	std::string target;
	iss >> target; //channel ou nickname

	if (target.empty())
	{
		sendReply(fd, "411", "", "No recipient given (PRIVMSG)");
		return;
	}

	std::string message;
	std::getline(iss, message);

	if (message.empty())
	{
		sendReply(fd, "412", "", "No text to send");
		return;
	}

	if (!message.empty() && message[0] == ' ')
		message.erase(0, 1);

	if (!message.empty() && message[0] == ':')
		message.erase(0, 1);

	Client* sender = _clients[fd];

	if (target[0] == '#' || target[0] == '&')
	{
		if (_channels.find(target) == _channels.end())
		{
			sendReply(fd, "403", target, "No such channel");
			return;
		}

		Channel* ch = _channels[target];

		const std::vector<Client*>& members = ch->getMembers();
		if (std::find(members.begin(), members.end(), sender) == members.end())
		{
			sendReply(fd, "404", target, "Cannot send to channel (not member)");
			return;
		}

		std::string full_msg = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname() + " PRIVMSG " + target + " :" + message;
		for (std::vector<Client*>::const_iterator it = members.begin(); it != members.end(); ++it) // envoi à tous les membres sauf à celui qui envoie le msg
		{
			Client* c = *it;
			if (c != sender)
				sendMessage(c->getFd(), full_msg);
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
			sendReply(fd, "401", target, "No such nick");
			return;
		}

		std::string full_msg = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname() + " PRIVMSG " + target + " :" + message;
		sendMessage(receiver->getFd(), full_msg);
	}
}

void Server::handleTopic(int fd, std::istringstream &iss)
{
	std::string channelName;
	iss >> channelName;

	if (channelName.empty())
	{
		sendReply(fd, "461", "TOPIC", "You must specify a channel name");
		return;
	}

	if (_channels.find(channelName) == _channels.end())
	{
		sendReply(fd, "403", channelName, "No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd];

	const std::vector<Client*>& members = ch->getMembers();

	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendReply(fd, "442", channelName, "You're not on that channel");
		return;
	}

	std::string topic;
	std::getline(iss, topic);

	if (!topic.empty() && topic[0] == ' ')
		topic.erase(0, 1);

	if (!topic.empty() && topic[0] == ':')
		topic.erase(0, 1);

	if (topic.empty())
	{
		if (ch->getTopic().empty())
			sendReply(fd, "331", channelName, "No topic set");
		else
			sendReply(fd, "332", channelName, ch->getTopic());
		return;
	}

	if (ch->isTopicRestricted() && !ch->isAdmin(client)  && !client->isOperator()) //modif du topic
	{
		sendReply(fd, "482", channelName, "You're not channel operator");
		return;
	}

	ch->setTopic(topic);

	std::string msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " TOPIC " + channelName + " :" + topic;;

	for (std::vector<Client*>::const_iterator it = ch->getMembers().begin(); it != ch->getMembers().end(); ++it)
	{
		Client* c = *it;
		sendMessage(c->getFd(), msg);
	}
}



void Server::handlePart(int fd, std::istringstream &iss)
{
	std::string channelName;
	iss >> channelName;

	if (channelName.empty())
	{
		sendReply(fd, "461", "PART", "You must specify a channel name");
		return;
	}

	if (_channels.find(channelName) == _channels.end())
	{
		sendReply(fd, "403", channelName, "No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd];

	const std::vector<Client*>& members = ch->getMembers();
	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendReply(fd, "442", channelName, "You're not on that channel");
		return;
	}

	std::string msg;
	std::getline(iss, msg);
	if (!msg.empty() && msg[0] == ':')
		msg.erase(0, 1);

	// prévenir tous les membres
	std::string notify = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " PART " + channelName;
	if (!msg.empty())
		notify += " :" + msg;

	for (std::vector<Client*>::const_iterator it = ch->getMembers().begin(); it != ch->getMembers().end(); ++it)
	{
		Client* c = *it;
		sendMessage(c->getFd(), notify);
	}

	ch->removeMember(client);

	if (ch->getMembers().empty())
	{
		delete ch;
		_channels.erase(channelName);
	}
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
		sendReply(fd, "461", "INVITE", "Not enough parameters, you must specify a nick and a channel");
		return;
	}

	if (_channels.find(channelName) == _channels.end())
	{
		sendReply(fd, "403", channelName, "No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd];

	const std::vector<Client*>& members = ch->getMembers(); //verifie si celui qui demande fait bien partie du channel
	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendReply(fd, "442", channelName, "You're not on that channel");
		return;
	}

	if (ch->isInviteOnly() && !ch->isAdmin(client)) // si channel est +i, seuls operators peuvent inviter
	{
		sendReply(fd, "482", channelName, "You're not channel operator");
		return;
	}

	Client* target = getClientByNick(nick); //cherche la cible dans le serveur
	if (!target)
	{
		sendReply(fd, "401", nick, "No such nick");
		return;
	}

	if (std::find(members.begin(), members.end(), target) != members.end()) //verifie si pas deja dans le channel
	{
		sendReply(fd, "443", nick + " " + channelName, "User already on channel");
		return;
	}

	ch->addInvited(target);

	std::string prefix = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();

	sendMessage(target->getFd(), prefix + " INVITE " + nick + " " + channelName); //envoi du msg d'invit à l'invité

	sendReply(fd, "341", nick + " " + channelName, ""); //confirmation d'envoi au client qui a fait la demande
}

void Server::handleMode(int fd, std::istringstream &iss)
{
	std::string channelName;
	iss >> channelName;

	if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
		return; //mode utilisateur a la connection, donc ignore

	if (_channels.find(channelName) == _channels.end())
	{
		sendReply(fd, "403", channelName, "No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd];

	const std::vector<Client*>& members = ch->getMembers();
	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendReply(fd, "442", channelName, "You're not on that channel");
		return;
	}

	if (!ch->isAdmin(client) && !client->isOperator())
	{
		sendReply(fd, "482", channelName, "You're not channel operator");
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
		sendReply(fd, "324", channelName, ch->getModes());
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
			//std::cout << "MODE +i set to " << add << std::endl; //debug
		}
		else if (m == 'o') // donner/retirer droits opérateur
		{
			if (paramIndex >= params.size())
			{
				sendReply(fd, "461", "MODE", "Not enough parameters");
				continue;
			}

			std::string targetNick = params[paramIndex++];
			Client* target = getClientByNick(targetNick);

			if (!target)
			{
				sendReply(fd, "401", targetNick, "No such nick");
				continue;
			}

			bool found = false;

			for (std::vector<Client*>::const_iterator it = members.begin();
	 			it != members.end(); ++it)
			{
				if (*it && (*it)->getNickname() == targetNick)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				sendReply(fd, "441", targetNick + " " + channelName, "They aren't on that channel");
				continue;
			}

			if (add)
				ch->addAdmin(target);
			else
			{
				if (ch->getAdmins().size() == 1 && ch->isAdmin(target))
	  			{
					sendReply(fd, "482", channelName, "Cannot remove last operator"); //debug
					continue;
				}
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
					sendReply(fd, "461", "MODE", "Not enough parameters");
					return;
				}

				key = params[paramIndex++];
				ch->setMode('k', key);
				paramValue.push_back(key);
			}
			else
				ch->setMode('k', "");

			modeStr += "k";
		}
		else if (m == 'l') // limite de clients
		{
			std::string limit;

			if (add)
			{
				if (paramIndex >= params.size())
				{
					sendReply(fd, "461", "MODE", "Not enough parameters");
					continue;
				}
				limit = params[paramIndex++];
				for (size_t j = 0; j < limit.size(); j++)
				{
					if (!std::isdigit(static_cast<unsigned char>(limit[j])))
   					{
						sendReply(fd, "472", "l", "Invalid limit");
						limit = "";
	   	 				break;
   					}
				}

				if (limit == "0")
				{
					sendReply(fd, "472", "l", "Invalid limit");
					continue;
				}

				if (limit.empty())
					continue;
			}
			else
				limit = "";

			ch->setMode('l', limit);

			if (!limit.empty())
				paramValue.push_back(limit);

			modeStr += "l";
		}
		else
			sendReply(fd, "472", std::string(1, m), "Unknown mode char");
	}

	std::string msg = ":" + client->getNickname()
					+ "!" + client->getUsername()
					+ "@" + client->getHostname()
					+ " MODE " + channelName
					+ " " + modeStr;
	for (size_t i = 0; i < paramValue.size(); i++)
		msg += " " + paramValue[i];

	for (std::vector<Client*>::const_iterator it = ch->getMembers().begin(); it != ch->getMembers().end(); ++it) //envoi à tous les membres
		sendMessage((*it)->getFd(), msg);
}


void Server::handleKick(int fd, std::istringstream &iss)
{
	std::string channelName, targetNick;
	iss >> channelName >> targetNick;

	if (channelName.empty() || targetNick.empty())
	{
		sendReply(fd, "461", "KICK", "Not enough parameters (must specify channel and user)");
		return;
	}

	if (_channels.find(channelName) == _channels.end())
	{
		sendReply(fd, "403", channelName, "No such channel");
		return;
	}

	Channel* ch = _channels[channelName];
	Client* client = _clients[fd]; // client qui fait la demande

	const std::vector<Client*>& members = ch->getMembers();
	if (std::find(members.begin(), members.end(), client) == members.end())
	{
		sendReply(fd, "442", channelName, "You're not on that channel");
		return;
	}

	if (!ch->isAdmin(client) && !client->isOperator())
	{
		sendReply(fd, "482", channelName, "You're not channel operator");
		return;
	}

	Client* target = getClientByNick(targetNick);  // récupérer le client à kicker
	if (!target || std::find(members.begin(), members.end(), target) == members.end())
	{
		sendReply(fd, "441", targetNick + " " + channelName, "They aren't on that channel");
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

	for (std::vector<Client*>::const_iterator it = members.begin(); it != members.end(); ++it) //info à tous les membres
	{
		Client* c = *it;
		sendMessage(c->getFd(), kick_msg);
	}

	ch->removeMember(target);

	if (ch->getMembers().empty()) //supp du channel si vide
		_channels.erase(channelName);
}

void Server::handlePing(int fd, std::istringstream &iss) //sans PONG le client (nc, irssi, etc.) pense que le serveur est mort, donc important
{
	std::string token;
	iss >> token;

	if (token.empty())
	{
		sendReply(fd, "409", "", "No origin specified");
		return;
	}

	if (token[0] == ':')
		token.erase(0, 1);
	sendMessage(fd, ":" + _serverName + " PONG :" + token);
}

