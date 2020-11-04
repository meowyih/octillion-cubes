# Login Server for Massive User
## Sinlge login server with multiple game servers

This project implements a real login server on Linux. It was written in C++17 without the 3rd library except openssl. I also have a blog post written in zh_TW: https://www.yhorng.com/blog/?p=266

# Secure Users' Password in Storage
## Encrypt / Hash Password

Rule#1, *never store any password in plain text!* Programmer should assume the data in server's permanent storage is never safe. Besides, most of the people uses less than three different passwords in everywhere. It is programmer's responsibility to secure it.

To hide user's password, we need to encrypt or hash the password before store it. The algorithm has to be one-way function because we have to assume hacker knows what the algorithm we use.

There are many hash algorithm can do that, like SHA or MD5, but most of them are not safe enough. Check the next section for details.

## Brute Force Attack

In case of hacker might knows what kind of algorithm we encrypt the password, they can actually use brutal force attack to try every possible combination. To prevent it, the encrypt/hash algorithm need to be slow, such as 0.5~1 second to run each function. We want hacker to get the real password as late as possible even using super powerful computer. The most common choice is bcrypt algorithm with 12+ factor.

## Rainbow Table Attack

Instead of brute force attack, there is another attack named rainbow table. Hacker pre-generates all the possible password combinations into a table. When they get any hashed password, they just search the table and find the real password.

To prevent this kind of attack, we need to add a long prefix or post-fix text in the plain password before hashing. The prefix/post-fix generates at runtime is called *salt*. And the hard-coded prefix/post-fix is called *pepper*. 'Salt' is must-to-have, but use both is a plus in the security perspective.

# Seperate Login Server and Game Sever

Hash/Encrpty password is very computing power consuming. Because of that, we really need to seperate the login logic and game logic into different severs. There is another alternative solution which is don't host the login server at all and using other 3rd party identity authentication service such as facebook or google. But that does not relate to this project.

## How it works? 

1. End-user sends their username and password to login sever via SSL.
2. If the end-user is valid, login server generate a long random token and send to end-user along with game server's address. Login server also records end-user's login the time, IP address and the random token.
3. End-user sends the token to the game server.
4. Game server sends end-user's name and token back to login server.
5. If the token is correct and has not been expired (usually 30 seconds at most), besides, the end-user's ip address is also matched, login server informs game server this end-user is a good one.

Every connection to the login server has to be protected, i.e. must under SSL. But the data commumication between game server and end-user could just normal socket.

# Dependency

## Login Server
```
macrolog -+
ocerror  -+- sslserver - loginserver - lserver
jsonw    ----------------+
event     ---------------+
dataqueue ---------------+
blowfish  ---------------+
```
## Game Server
```
macrolog -+
ocerror  -+- server - gameserver - gserver
sslclient ------------+
jsonw     ------------+
event     ------------+
dataqueue ------------+
```
note: Enable TEST_LOGIN_MECHANISM_ONLY macro for gamerserver to disable the game logic.

## End-User client
```
macrolog -+- client
ocerror  -+
sslclient +
jsonw     +
event     +
dataqueue +
```
### sslclient: simple block mode ssl client
### macrolog: Log system
### ocerror: error code
### jsonw: Json tool
### dataqueue: every data block start with 4 bytes length followed by payload
### event: command code
### blowfish: bcrypt function
