This project has been created as part of the 42 curriculum by ofilloux[, lylfergu[.

**DESCRIPTION**

The ft_irc project consists of developing an IRC server while adhering to the IRC protocol and using the C++98 programming language. To enable smooth client communication, the server must manage multiple connections at once and operate in a non-blocking mode using a single poll() loop.
The server communicates using the TCP/IP protocol and can be used with a standard IRC client. Several IRC clients exist, we decided to use Irssi as our reference client.

The server supports the core features of an IRC network, including:

- client authentication with password protection,
- nickname and username registration,
- channel creation and management,
- private and channel messaging,
- multiple clients connected at the same time,
- operator and user roles,
- channel operator commands such as KICK, INVITE, TOPIC, and MODE.

Several channel modes are implemented:

+i → invite-only channel,
+t → topic modification restricted to operators,
+k → password-protected channel,
+o → operator privileges,
+l → user limit.


**INSTRUCTIONS**

1. Compilation

Use the Makefile to compile the project with the required compilation flags.
The project must compile using the C++98 standard with the flags -Wall -Wextra -Werror.

2. Start the server

./ircserv <port> <password>

Arguments:
<port>: The port number on which the server will listen for incoming connections.
<password>: The password that clients must provide to connect to the server.

3. Connect to an IRC Server

Test with netcat :

nc localhost <port>
PASS <password>
USER bob
NICK bobby
JOIN #test
PRIVMSG #test :Hello World!
QUIT

-----------------------

Connection with an IRC client: irssi (recommended for testing)

Launch irssi
/CONNECT localhost <port> <password>
/USER Tom
/NICK tomtom
/JOIN #general
/MSG #general Hey guys !


//CONNECTION COMMANDS

| Command | Description                    | Syntax                                    |
| ------- | ------------------------------ | ----------------------------------------- |
| `PASS`  | Server password authentication | `PASS <password>`                         |
| `NICK`  | Set the user's nickname        | `NICK <nickname>`                         |
| `USER`  | Set user information           | `USER <user>`                             |
| `QUIT`  | Disconnect from the server     | `QUIT [:<message>]`                       |

//CHANNEL COMMANDS

| Command   | Description                       | Syntax                                   |
| --------- | --------------------------------- | ---------------------------------------- |
| `JOIN`    | Join a channel                    | 
`/join <#channel> [<key>]`                |
| `PART`    | Leave a channel                   | 
`/part <#channel>`                        |
| `MODE`    | Modify channel modes              | 
`/mode <#channel> <modes> [<params>]`     |
| `TOPIC`   | View or change the channel topic  | 
`/topic <#channel> :<topic>`              |
| `INVITE`  | Invite a user to a channel        | 
`/invite <nickname> <#channel>`           |
| `KICK`    | Remove a user from a channel      | 
`/kick <#channel> <nickname>`             |
| `PRIVMSG` | Send a private or channel message | 
`/msg <target> :<message>`            |


//DEBUG AND DIAGNOSTIC TOOLS

Check active connections on port <port>:

lsof -i :<port>

Forcefully close processes using port <port>:

kill -9 $(lsof -t -i :<port>)

Memory leaks and active file descriptor check for ircserv using Valgrind:

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes ./ircserv <port> <password>


**RESOURCES**

https://modern.ircdocs.horse/

https://datatracker.ietf.org/doc/html/rfc2812

https://www.csd.uoc.gr/~hy556/material/tutorials/cs556-3rd-tutorial.pdf
