
+----------------------------------+
|          world module            |
+----------------+-----------------+
|  server module | database module |
+----------------+-----------------+
|           error module           |
+----------------------------------+



LOGIN Process
=============
a) Client requests new character

-- Client sends RESERVED_PCID
   Server create new PCID -> create Player / save file
   Server add fd to Player object mapping
   Server return PCID to client
   
-- Client roll username (ROLL_USENAME)
-- Client confirm the username / cls / gender
   Client decides the password based on MAC / osunique id (hash)
   
-- Client logout in CREATING states, Server should delete their save file and mapping


   Server   
-- Client roll character
   Server write character attribute into Player obj and save
-- Client confirm the result
   Server save the userid / password / pcid mapping in database
   
TCP connect
Client 