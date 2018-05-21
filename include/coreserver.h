#ifndef OCTILLION_CORE_SERVER_HEADER
#define OCTILLION_CORE_SERVER_HEADER

#include <string>
#include <thread>
#include <system_error>
#include <map>

#include <openssl/ssl.h>

#include "coreservercallback.h"

namespace octillion
{
    class CoreServer;
}

// CodeServer definition
class octillion::CoreServer 
{
    private:
        const std::string tag_ = "CoreServer";
        
    // singleton
    public:
        static CoreServer& get_instance()
        {
            static CoreServer instance;
            return instance;
        }
        
    public:        
        // start the server thread
        std::error_code start( std::string port );
        
        // raise the stop flag and wait until the server thread die
        std::error_code stop();
        
        // send data via a socket fd
        std::error_code senddata( int socketfd, const void *buf, size_t len );
        
        // send data via a socket fd
        void closesocket( int socketfd );
        
        // check if server thread is still running
        bool is_running() { return is_running_; }
        
        // assign callback class, multiple callback instances is not supported
        void set_callback( CoreServerCallback* callback ) { callback_ = callback; }
        
    private:
        CoreServer();
        ~CoreServer();
        
        void core_task();
        
        std::error_code init_server_socket();
        std::error_code set_nonblocking( int fd );

    private: // SSL usage
        std::map<int, SSL*> ssl_;        
    
    private:
        // epoll timeout
        int epoll_timeout_;        
        
        // epoll event buffer size
        int epoll_buffer_size_;
        
        CoreServerCallback* callback_;
        
    public:                
        // avoid accidentally copy
        CoreServer( CoreServer const& ) = delete;
        void operator = ( CoreServer const& ) = delete;        

    
    private:
        std::string port_;
        int server_fd_;
        int epoll_fd_;
                
        bool is_running_;
        bool core_thread_flag_;
        
        std::thread* core_thread_;
    
    private:
        const int kEpollTimeout = 5 * 1000;
        const int kEpollBufferSize = 64;
        
};

#endif // OCTILLION_CORE_SERVER_HEADER
