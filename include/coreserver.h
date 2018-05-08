#ifndef OCTILLION_CORE_SERVER_HEADER
#define OCTILLION_CORE_SERVER_HEADER

#include <string>
#include <system_error>

#include "coreservercallback.h"

namespace octillion
{
    class CoreServer;
}

// CodeServer definition
class octillion::CoreServer 
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

#endif // OCTILLION_CORE_SERVER_HEADER