
This project has been created as part
of the 42 curriculum by ofilloux[, lyl[


///////Description//////


//////Instructions//////

//////Resources//////












_________________________TO TRANSLATE____________________________

/////// 1. CHOIX DU CLIENT IRC /////////

Client	| Type		| OS		| Pourquoi pour ft_irc ?
-------------------------------------------------------------------------------------
Irssi	| Terminal	| Linux/WSL	| Pour valider la conformité stricte au RFC.
HexChat	| Graphique	| Linux/Win	| Pour tester l'expérience utilisateur et les canaux.
----------------------------------------

💡 L'outil indispensable : Netcat (nc) (man nc)
Avant même de choisir un client, ton meilleur ami sera Netcat. En tapant nc 127.0.0.1 6667, tu peux envoyer manuellement tes commandes (ex: NICK flo) et voir exactement ce que ton serveur renvoie sans aucun filtre. C'est l'étape 0 de tout bon développeur d'IRC.

Conseil : Commencer par Netcat au debut, puis passer sur Irssi pour coder les commandes de base, et termine avec HexChat pour t'assurer que le serveur est "propre" visuellement.
