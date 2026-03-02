#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "ft_irc.hpp"

class Client;

class Channel
{
    private:
    std::string _name;
    std::vector<Client*> _members;      // Qui est dedans ?
    std::vector<Client*> _admins;       // Qui sont les opérateurs ?
    std::string _topic;
    std::map<char, std::string> _modes;  // i, t, k, o, l... (map pour gerer plus facilement les differents modes actives)(bool = etat actif/inactif)

    public:
    Channel(std::string name);
    void addMember(Client* client);
    void removeMember(Client* client);
    bool isAdmin(Client* client) const;
};

  #endif