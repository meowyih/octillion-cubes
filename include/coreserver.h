
#include <string>
#include <system_error>

#include "coreservercallback.h"

#ifndef CORE_SERVER_HEADER
#define CORE_SERVER_HEADER


// CodeServer definition
class CoreServer 
{
    public:
        // init the core server with specific port
        CoreServer( std::string port );
        
        ~CoreServer();
        
        // start the server thread
        std::error_code start();
        
        // raise the stop flag and wait until the server thread die
        std::error_code stop();
        
        // check if server thread is still running
        bool is_running() { return is_running_; }
        
        // assign callback class, multiple callback instances is not supported
        void set_callback( CoreServerCallback* callback ) { callback_ = callback; }
        
    private:
        void core_task();
        
        std::error_code init_server_socket();
        std::error_code set_nonblocking( int fd );
        
    private:
        // epoll timeout
        int epoll_timeout_;
        
        // epoll event buffer size
        int epoll_buffer_size_;
        
        CoreServerCallback* callback_;
    
    private:
        std::string port_;
        int server_fd_;
        int epoll_fd_;
                
        bool is_running_;
        bool core_thread_flag_;
        
        std::thread* core_thread_;
};

// inject error code into std:error
enum class CoreServerError
{
    E_SUCCESS = 0,
    E_SERVER_BUSY = 10,
    
    
    E_SYS_GETADDRINFO = 20,
    E_SYS_BIND = 30,
    E_SYS_FCNTL = 40,
    E_SYS_LISTEN = 50,
    E_SYS_EPOLL_CREATE = 60,
    E_SYS_EPOLL_CTL = 70,
    
    E_FATAL = 999,    
};

namespace std
{
    template<> struct is_error_code_enum<CoreServerError> : true_type {};
}
 
std::error_code make_error_code( CoreServerError );

#endif // CORE_SERVER_HEADER