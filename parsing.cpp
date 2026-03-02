#include "Server.hpp"


bool Server::isValidNickname(std::string& nickname)
{
	if(!nickname.empty() && (nickname[0] == '&' || nickname[0] == '#' || nickname[0] == ':')) //on exclue le cas des channels
		return (false);
	for(size_t i = 1; i < nickname.size(); i++)
	{
		if(!std::isalnum(nickname[i]) && nickname[i] != '_') //a-z ou 0-9
			return (false);
	}
	return (true);
}


bool Server::nicknameUsed(std::string& nickname) //pour eviter doublons de noms
{
	for (size_t i = 0; i < this->client.size(); i++)
	{
		if (this->client[i].getNickname() == nickname)
			return (true);
	}
	return (false);
}



/*checkNickname : Les surnoms sont des chaînes de caractères non vides soumises aux restrictions suivantes :

Ils ne doivent contenir aucun des caractères suivants : espace (' ', 0x20), virgule (',', 0x2C), astérisque ('*', 0x2A), point d’interrogation ('?', 0x3F), point d’exclamation ('!', 0x21), arobase ('@', 0x40).
Ils ne doivent PAS commencer par l'un des caractères suivants : dollar ('$', 0x24), deux-points (':', 0x3A).
Ils ne doivent PAS commencer par un caractère répertorié comme type de canal , préfixe d'appartenance à un canal ou préfixe répertorié dans l' multi-prefixextension IRCv3 .
Ils ne doivent contenir aucun point ('.', 0x2E).

Les serveurs PEUVENT avoir des restrictions supplémentaires concernant les surnoms, spécifiques à leur implémentation, et DEVRAIENT éviter l'utilisation de surnoms ambigus avec les commandes ou les paramètres de commande, car cela pourrait entraîner de la confusion ou des erreurs.


checkChannel : Les noms de canaux sont des chaînes de caractères 
(commençant par des caractères de préfixe spécifiques). 
Outre l'exigence que le premier caractère soit un préfixe de type de canal valide , 
la seule restriction concernant un nom de canal est qu'il 
ne peut contenir ni espaces (' ', 0x20), ni le caractère de contrôle G / BELL ('^G', 0x07), ni de virgule (',', 0x2C)(utilisée comme séparateur d'éléments de liste par le protocole).

parsing msg: 
Un message IRC est une seule ligne, délimitée par une paire de caractères CR 
('\r', 0x0D)« et LF ('\n', 0x0A)».

Les noms des entités IRC (clients, serveurs, canaux) sont soumis à une correspondance de casse. Cela évite, par exemple, qu'un utilisateur ait le pseudo « clients » 'Dan'et un autre le pseudo « canaux » 
'dan', ce qui pourrait induire les autres utilisateurs en erreur. 

Lors de la lecture de messages provenant d'un flux, chargez les données entrantes dans une mémoire tampon. Ne traitez un message qu'à partir du moment où vous rencontrez le \r\ncaractère de fin. 
Si vous rencontrez un message vide, ignorez-le silencieusement.

Lors de l'envoi de messages, assurez-vous qu'une paire 
de \r\ncaractères suive chaque message envoyé par votre logiciel.


Les serveurs DOIVENT gérer \nles caractères isolés et PEUVENT les traiter \rcomme s'il s'agissait d'une \r\npaire, afin de prendre en charge les clients existants susceptibles d'envoyer ce type de caractères. Cependant, ni les clients ni les serveurs ne DOIVENT envoyer \rde \ncaractères isolés.

Les serveurs et les clients DOIVENT ignorer les lignes vides.

Les serveurs DEVRAIENT gérer correctement les messages dépassant la limite de 512 octets. Ils peuvent :

Renvoyer un code d'erreur numérique, de préférenceERR_INPUTTOOLONG (417)
Tronquer au 510e octet (et ajouter \r\nà la fin) ou, de préférence, au dernier caractère ou graphème UTF-8 qui tient.
Ignorez le message ou fermez la connexion – mais cela peut prêter à confusion pour les utilisateurs de clients bogués.
Enfin, les clients et les serveurs NE DOIVENT PAS utiliser plus d'un \x20caractère espace ( ) comme SPACEdéfini dans la grammaire ci-dessus./

*/