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

//commandes channel

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

    Channel* ch = _channels[channelName]; //on recup l'obj channel
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

    std::string prefix = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();

    sendMessage(target->getFd(), prefix + " INVITE " + nick + " " + channelName); //envoi du msg d'invit à l'invité

    sendMessage(fd, "341 " + client->getNickname() + " " + nick + " " + channelName); //confirmation d'emvoi au client qui a fait la demande
}




void Server::handleMode(int fd, std::istringstream &iss)
{
    std::string channelName;
    iss >> channelName;

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

    if (mode.empty())
    {
        sendMessage(fd, "324 " + channelName + " :" + ch->getModes());
        return;
    }

    bool add = true;
    std::vector<std::string> paramModes; // pour construire le message complet

    for (size_t i = 0; i < mode.size(); i++)
    {
        char m = mode[i];

        if (m == '+')
            add = true;
        else if (m == '-')
            add = false;
        else if (m == 't')
            ch->setTopicRestriction(add), paramModes.push_back(std::string(1, m));
        else if (m == 'i')
            ch->setInviteOnly(add), paramModes.push_back(std::string(1, m));
        else if (m == 'o') // donner/retirer droits opérateur
        {
            std::string targetNick;
            iss >> targetNick;
            if (targetNick.empty())
            {
                sendMessage(fd, "461 MODE :Not enough parameters");
                continue;
            }
            Client* target = getClientByNick(targetNick);
            if (!target || std::find(members.begin(), members.end(), target) == members.end())
            {
                sendMessage(fd, "441 " + targetNick + " " + channelName + " :They aren't on that channel");
                continue;
            }
            if (add)
                ch->addAdmin(target);
            else
                ch->removeAdmin(target);
            paramModes.push_back(std::string(1, m) + " " + targetNick);
        }
        else if (m == 'k') // mot de passe channel
        {
            std::string key;
            iss >> key;
            if (add && key.empty())
            {
                sendMessage(fd, "461 MODE :Not enough parameters");
                continue;
            }
            if (!add)
                key = "";

            ch->setMode('k', key);

            std::string msgPart = "k";
            if (!key.empty())
                msgPart += " " + key;
            paramModes.push_back(msgPart);
        }
        else if (m == 'l') // limite de clients
        {
            std::string limit;
            iss >> limit;
            if (add && limit.empty())
            {
                sendMessage(fd, "461 MODE :Not enough parameters");
                continue;
            }
            if (!add)
                limit = "";

            ch->setMode('l', limit);

            std::string msgPart = "l";
            if (!limit.empty())
                msgPart += " " + limit;
            paramModes.push_back(msgPart);
        }
        else
        {
            sendMessage(fd, "472 " + std::string(1, m) + " :Unknown mode char");
        }
    }

    std::string msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " MODE " + channelName;
    for (size_t i = 0; i < paramModes.size(); i++)
        msg += " " + paramModes[i];
    msg += "\r\n";

    for (std::vector<Client*>::const_iterator it = ch->getMembers().begin(); it != ch->getMembers().end(); ++it) //envoi à tous les membres
    {
        Client* c = *it;
        send(c->getFd(), msg.c_str(), msg.size(), 0);
    }
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
