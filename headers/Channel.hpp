#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "ft_irc.hpp"

class Client;

class Channel
{
	private:
	std::string _name;
	std::vector<Client*> _members;	  // Qui est dedans ?
	std::vector<Client*> _admins;	   // Qui sont les opérateurs ?
	std::vector<Client*> _invitedList; // invites autorisés a rejoindre le canal
	std::string _topic;
	bool _topicRestricted; //+t
	bool _inviteOnly; //+i
	int _limit; //pour setmode
	std::string _key;
	std::map<char, std::string> _modes; //map pour gerer plus facilement les differents MODES avec parametres (o, k, ...)

	public:
	Channel(std::string name);
	void	addMember(Client *client);
	void	removeMember(Client *client);
	bool	isInvited(Client *client);
	void	addInvited(Client *client);
	void	removeInvited(Client *client);
	void	addAdmin(Client *c);
	void	removeAdmin(Client *c); // mode o

	//getters
	bool		isAdmin(Client* client) const;
	bool		isTopicRestricted() const;
	bool		isInviteOnly() const;
	bool 		isEmpty() const;
	const std::vector<Client*>& getMembers() const;
	const std::vector<Client*>& getAdmins() const;
	std::string	getName() const;
	std::string	getTopic() const;
	std::string	getModes() const; //renvoie les modes actifs et leurs paramètres pour le MODE #channel sans paramètre.
	std::string	getKey() const;
	int			getLimit() const;

	//setters
	void	setTopic(const std::string &topic);
	void	setInviteOnly(bool value);
	void	setTopicRestriction(bool value);
	void	setMode(char m, const std::string &param); // pour k et l

};

#endif
