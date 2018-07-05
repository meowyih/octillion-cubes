
# Octillion Cubes Project

Author   | Email
---------|------------------
Yih Horng|yhorng75@gmail.com

# Basic Architecture

Two modules run on two different thread. Server module provides *RawProcessor::senddata()* for world module to send data to clients. World module has *World::addcmd()* method to accept the clients' input. Other than these two method, two modules are totally seperated. 

+-----------------------------+
|World Module                 |
+-----------------------+-----+
  World::addcmd()       |
        |               |
        |   RawProcessor::senddata()
+-------+---------------------+
|Server Module                |
+-----------------------------+

Both modules are singleton pattern. Caller can get instance by *get_instance()* method. Initialization is not necessary.

``` c++

    // get core server's instance
    octillion::CoreServer::get_instance();
    
    // get world's instance
    octillion::World::get_instance();
    
```

# Server Module 

\server\coreserver
\server\rawprocessor

+---------------------------+
|     CoreServer            |
|+-------------------------+| 
|| set_callback(           ||  +---------------------------------+
||    CoreServerCallback  <----+ RawProcessor:CoreServerCallback |
|| )                       ||  +---------------------------------+
|+-------------------------+|
+---------------------------+

*CoreServer* provides CoreServerCallback interface to notify the network connect, recv and disconnect event.

``` c++
    class CoreServerCallback
    {       
        public:
            virtual void connect( int fd ) = 0;
            virtual int recv( int fd, uint8_t* data, size_t datasize) = 0;
            virtual void disconnect( int fd ) = 0;
    };
```

*RawProcessor* inherits *CoreServerCallback* to process the incoming data via *recv()* into *Command*. Then calls *World::get_instance().addcmd()* to send command to World module. Similar to *recv()*, RawProcessor also converts *connect()* and *disconnect()* into Command and notify *World*. *RawProcessor* also provide static method *RawProcessor::senddata()* to let World module to send data to client.

# World Module

\world\world
\world\command
\world\event
\world\player
\world\cube

The core method for the World module is World::tick(). The world thread need to continually call tick() to push world go forward. Without calling tick(), the world will be freezed and all the Commands received from Server module will be stored in World::cmds_ list.
