#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(std::string name):
				_name(name),
				_topic(""),
				_topicRestricted(false),
				_inviteOnly(false),
				_limit(0),
				_key("") {}

void Channel::addMember(Client* client)
{
	if (!client)
		return;

	if (std::find(_members.begin(), _members.end(), client) != _members.end())
		return;

	_members.push_back(client);
	if (_members.size() == 1) // Si premier membre, devient admin automatiquement
		_admins.push_back(client);
}


void Channel::removeMember(Client* client)
{
	if (!client)
		return;

	std::vector<Client*>::iterator it;

	for (it = _members.begin(); it != _members.end(); )
	{
		if (*it == client)
			it = _members.erase(it);
		else
			++it;
	}

	for (it = _admins.begin(); it != _admins.end(); )
	{
		if (*it == client)
			it = _admins.erase(it);
		else
			++it;
	}

	if (_admins.empty() && !_members.empty())
		_admins.push_back(_members[0]); //si plus d'admin, on donne admin au premier
}

bool	Channel::isInvited(Client *client)
{
	if(std::find(_invitedList.begin(), _invitedList.end(), client) == _invitedList.end())
		return false;
	return true;
}

void	Channel::addInvited(Client *client)
{
	if (std::find(_invitedList.begin(), _invitedList.end(), client) == _invitedList.end())
		_invitedList.push_back(client);
}

void	Channel::removeInvited(Client *client)
{
	std::vector<Client*>::iterator it = std::find(_invitedList.begin(), _invitedList.end(), client);
	if (it != _invitedList.end())
		_invitedList.erase(it);
}


//methodes pour MODE
void Channel::addAdmin(Client* c)
{
	if (std::find(_admins.begin(), _admins.end(), c) == _admins.end())
		_admins.push_back(c);
}

void Channel::removeAdmin(Client* c)
{
	if (_members.size() <= 1)
		return;

	std::vector<Client*>::iterator it = std::find(_admins.begin(), _admins.end(), c);
	if (it != _admins.end())
		_admins.erase(it);

	if (_admins.empty())
	{
		// promotion du premier membre en tant qu'admin si il n'y a plus d'admin
		for (size_t i = 0; i < _members.size(); i++)
		{
			if (_members[i] != c)
			{
				_admins.push_back(_members[i]);
				break;
			}
		}
	}
}


//getters
bool Channel::isAdmin(Client* client) const
{
	if (!client)
		return (false);
	return (std::find(_admins.begin(), _admins.end(), client) != _admins.end());
}

bool Channel::isEmpty() const
{
    return _members.empty();
}


const std::vector<Client*>& Channel::getMembers() const
{
	return _members;
}

const std::vector<Client*>& Channel::getAdmins() const
{
	return _admins;
}


std::string Channel::getName() const
{
	return _name;
}


std::string Channel::getTopic() const
{
	return _topic;
}

bool Channel::isTopicRestricted() const
{
	return _topicRestricted;
}

bool Channel::isInviteOnly() const
{
	return _inviteOnly;
}

std::string Channel::getModes() const
{
	std::string res;
	if (_topicRestricted)
		res += "t";
	if (_inviteOnly)
		res += "i";
	return ("+" + res);
}

std::string Channel::getKey() const
{
	return (_key);
}

int Channel::getLimit() const
{
	return (_limit);
}

//setters
void Channel::setTopic(const std::string &topic)
{
	_topic = topic;
}

void Channel::setInviteOnly(bool value)
{
	_inviteOnly = value;
}

void Channel::setTopicRestriction(bool value)
{
	_topicRestricted = value;
}


void Channel::setMode(char mode, const std::string &param)
{
	if (mode == 'k') // mot de passe
		_key = param;

	else if (mode == 'l') // limite de clients
	{
		if (param.empty())
			_limit = 0;

		else
		{
			std::stringstream ss(param); //crée ss avec contenu de param(permet de lire et ecrire dans le flux)

			int value = 0;
			ss >> value; //conversion en int si input valide

			if (ss.fail() || !ss.eof() || value < 0) // checke si conversion a échoué, s'il reste des caractères apres l entier ou si neg
			{
				_limit = 0;
			}
			else
			{
				_limit = value;
			}
		}
	}
}
