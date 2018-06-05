#ifndef OCTILLION_CORE_SERVER_HEADER
#define OCTILLION_CORE_SERVER_HEADER

#include <mutex>
#include <string>
#include <thread>
#include <system_error>
#include <map>
#include <list>
#include <cstdint>

#ifdef WIN32
// Currently no Win32 Implementation for coreserver
using SSL = void;
#else
#include <openssl/ssl.h>
#endif

// note: differet way to handle SSL_ERROR_WANT_READ/SSL_ERROR_WANT_WRITE
// 1. non-blocking SSL_read needs to read several times 
//    until SSL_ERROR_WANT_READ/SSL_ERROR_WANT_WRITE
// 2. SSL_write without set SSL_MODE_ENABLE_PARTIAL_WRITE by SSL_CTX_set_mode
//    indicate only return success if whole data were sent. but if see 
//    SSL_ERROR_WANT_READ/SSL_ERROR_WANT_WRITE we still need to retry due to 
//    the conflict between SSL_read and SSL_write in CoreServer implementation, 
//    we put the retry logic in the same thread of core_task. It will make sure
//    won't have confliction in retry.

namespace octillion
{
    class CoreServerCallback;
    class CoreServer;
}

// CoreServerCallback definition
class octillion::CoreServerCallback
{
    private:
        const std::string tag_ = "CoreServerCallback";
        
    public:
        ~CoreServerCallback() {}
        
    public:
        // pure virtual function that handlers all incoming event
        virtual void connect( int fd ) = 0;
        
        // return 0 to close the fd due to invalid data
        virtual int recv( int fd, uint8_t* data, size_t datasize) = 0;
        virtual void disconnect( int fd ) = 0;
};

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
        std::error_code start( std::string port, std::string key, std::string cert );
        
        // raise the stop flag and wait until the server thread die
        std::error_code stop();
        
        // send data via a fd
        std::error_code senddata( int fd, const void *buf, size_t len, bool autoretry = true );

        // check if server thread is still running
        bool is_running() { return is_running_; }
        
        // assign callback class, multiple callback instances is not supported
        void set_callback( CoreServerCallback* callback ) { callback_ = callback; }
        
    private:
        CoreServer();
        ~CoreServer();        
        
        // send data via a socket fd, should be done in the core_task thread
        // to prevent wrong usage, put it in private
        void closesocket( int socketfd );
        
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
        std::string key_;
        std::string cert_;
        int server_fd_;
        int epoll_fd_;
        SSL* server_ssl_; // a pointer to ssl_ that can access server's ssl quicker
                          // no need to SSL_free( server_ssl_ ) it directly
                
        bool is_running_;
        bool core_thread_flag_;
        
        std::thread* core_thread_;
        
    private:
        // retry SSL_write buffer
        struct DataBuffer
        {
            int fd;
            uint8_t* data;
            size_t datalen;
        };
        
        std::list<DataBuffer> list_;
        std::mutex list_lock_;
    
    private:
        const int kEpollTimeout = 5 * 1000;
        const int kEpollBufferSize = 64;
        
};

#endif // OCTILLION_CORE_SERVER_HEADER
