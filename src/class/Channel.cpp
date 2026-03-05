#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(std::string name): _name(name), _topic(""){}


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

    _members.erase(std::remove(_members.begin(), _members.end(), client),_members.end()); //on parcourt le vecteur, retire le client, puis on efface l'espace a la fin laissé suite à ce retrait

    _admins.erase(std::remove(_admins.begin(), _admins.end(), client),_admins.end());
    if (_admins.empty() && !_members.empty())
        _admins.push_back(_members[0]); //si plus d'admin, on donne admin au premier
}


bool Channel::isAdmin(Client* client) const
{
    if (!client)
        return (false);
    return (std::find(_admins.begin(), _admins.end(), client) != _admins.end());
}

// getter membres
std::vector<Client*> Channel::getMembers() const
{
    return _members;
}

// getter topic
std::string Channel::getTopic() const
{
    return _topic;
}