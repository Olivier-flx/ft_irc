
#ifndef FT_IRC_H
# define FT_IRC_H

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#endif

// socket(int domain, int type, int protocol);
// Crée une socket réseau.
// domain : famille d’adresses (AF_INET, AF_INET6…)
// type : type de communication (SOCK_STREAM = TCP, SOCK_DGRAM = UDP)
// protocol : généralement 0 pour choix automatique.
// Retourne un descripteur de socket ou -1 si erreur.


// close(int fd);


// setsockopt(int sockfd, int level, int optname,
//            const void *optval, socklen_t optlen);
// Configure des options sur une socket (timeouts, reuse address, buffer…).


// getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// Récupère l’adresse locale associée à une socket.


// getprotobyname(const char *name);
// Retourne des infos sur un protocole réseau à partir de son nom ("tcp","udp").
// Obsolète dans beaucoup de cas modernes.


// gethostbyname(const char *name);
// Résolution DNS ancienne génération.
// Transforme un nom d’hôte en adresse IP. Remplacé par getaddrinfo().


// getaddrinfo(const char *node, const char *service,
//             const struct addrinfo *hints,
//             struct addrinfo **res);
// Résolution moderne nom ou service vers adresse réseau.
// Compatible IPv4 et IPv6.


// freeaddrinfo(struct addrinfo *res);
// Libère la mémoire allouée par getaddrinfo().


// bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
// Associe une socket à une adresse locale (IP + port).
// Indispensable côté serveur.


// connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
// Initie une connexion vers une adresse distante.
// Typiquement côté client TCP.


// listen(int sockfd, int backlog);
// Passe la socket en mode écoute.
// backlog définit le nombre max de connexions en attente.


// accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// Accepte une connexion entrante.
// Retourne une nouvelle socket dédiée au client.


// htons(uint16_t hostshort);
// Convertit un entier 16 bits du format machine vers format réseau
// (Host TO Network Short).


// htonl(uint32_t hostlong);
// Conversion 32 bits host vers network.


// ntohs(uint16_t netshort);
// Conversion inverse réseau vers machine (16 bits).


// ntohl(uint32_t netlong);
// Conversion inverse réseau vers machine (32 bits).


// inet_addr(const char *cp);
// Convertit une adresse IPv4 texte ("192.168.0.1")
// en format binaire. Ancien, inet_pton préféré.


// inet_ntoa(struct in_addr in);
// Convertit IPv4 binaire vers texte.
// Non thread-safe, inet_ntop préférable.


// inet_ntop(int af, const void *src, char *dst, socklen_t size);
// Conversion binaire vers texte (IPv4/IPv6 moderne).


// send(int sockfd, const void *buf, size_t len, int flags);
// Envoie des données sur une socket connectée.


// recv(int sockfd, void *buf, size_t len, int flags);
// Reçoit des données depuis une socket.


// signal(int signum, sighandler_t handler);
// Installe un gestionnaire simple pour un signal UNIX.
// Interface ancienne.


// sigaction(int signum, const struct sigaction *act,
//           struct sigaction *oldact);
// Gestion avancée des signaux.
// Permet plus de contrôle que signal().


// sigemptyset(sigset_t *set);
// Initialise un ensemble de signaux vide.


// sigfillset(sigset_t *set);
// Initialise un ensemble contenant tous les signaux.


// sigaddset(sigset_t *set, int signum);
// Ajoute un signal à un ensemble.


// sigdelset(sigset_t *set, int signum);
// Retire un signal d’un ensemble.


// sigismember(const sigset_t *set, int signum);
// Vérifie si un signal appartient à un ensemble.


// lseek(int fd, off_t offset, int whence);
// Déplace la position courante dans un fichier.
// whence : SEEK_SET, SEEK_CUR, SEEK_END.


// fstat(int fd, struct stat *buf);
// Récupère les infos d’un fichier ouvert
// (taille, permissions, dates…).


// fcntl(int fd, int cmd, ...);
// Contrôle divers sur descripteurs : flags,
// verrouillage de fichier, mode non bloquant, etc.


// poll(struct pollfd *fds, nfds_t nfds, int timeout);
// Surveille plusieurs descripteurs simultanément.
// Permet d’attendre événements I/O sans blocage total.

//poll Equivalents select(), kqueue(), or epoll()
