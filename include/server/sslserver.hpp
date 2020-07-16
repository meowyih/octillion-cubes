#ifndef OCTILLION_SSL_SERVER_HEADER
#define OCTILLION_SSL_SERVER_HEADER

#include <mutex>
#include <string>
#include <thread>
#include <system_error>
#include <map>
#include <list>
#include <cstdint>

#include <openssl/ssl.h>

namespace octillion
{
    class SslServerCallback;
    class SslServer;
}

// SslServerCallback definition
class octillion::SslServerCallback
{
    private:
        const std::string tag_ = "SslServerCallback";
        
    public:
        ~SslServerCallback() {}
        
    public:
        // pure virtual function that handlers all incoming event
        virtual void connect( int fd ) = 0;
        
        // return 0 to close the fd due to invalid data
        virtual int recv( int fd, uint8_t* data, size_t datasize) = 0;
        virtual void disconnect( int fd ) = 0;
};

// CodeServer definition
class octillion::SslServer 
{
    private:
        const std::string tag_ = "SslServer";
        
    // singleton
    public:
        static SslServer& get_instance()
        {
            static SslServer instance;
            return instance;
        }
        
    public:        
        // start the server thread
        std::error_code start( std::string port, std::string key, std::string cert );
        
        // raise the stop flag and wait until the server thread die
        std::error_code stop();

        // send data vid a fd. this function is thread safe
        std::error_code senddata( int fd, const void *buf, size_t len );

        // add fd into close queue and will be closed later in core_task thread
        std::error_code requestclosefd(int fd);

        // check if server thread is still running
        bool is_running() { return is_running_; }
        
        // assign callback class, multiple callback instances is not supported
        void set_callback( SslServerCallback* callback ) { callback_ = callback; }
        
    private:
        SslServer();
        ~SslServer();        
        
        // send data via a socket fd, should be done in the core_task thread
        // to prevent wrong usage, put it in private
        void closesocket( int socketfd );
        
        void core_task();
        
        std::error_code init_server_socket();
        std::error_code set_nonblocking( int fd );

    private: // SSL usage
        static std::string get_openssl_err( int sslerr );
        static std::string get_epoll_event( uint32_t event );
    
    private:
        // epoll timeout
        int epoll_timeout_;        
        
        // epoll event buffer size
        int epoll_buffer_size_;
        
        SslServerCallback* callback_;
        
    public:                
        // avoid accidentally copy
        SslServer( SslServer const& ) = delete;
        void operator = ( SslServer const& ) = delete;

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
        
        std::thread*    core_thread_;
        
    private:
        // SSL_write data buffer
        struct DataBuffer
        {
            int fd;
            uint8_t* data;
            size_t datalen;
        };
                
        std::list<DataBuffer> out_data_;
        std::mutex out_data_lock_;
        
        struct Socket
        {
            int fd;
            bool writable;
            unsigned long s_addr;
            SSL* ssl;
        };
        std::map<int,Socket> socket_list_;

        // fd that waiting for close
        std::mutex badfds_lock_;
        std::list<int> badfds_;
    
    private:
        const int kEpollTimeout = 5 * 1000;
        const int kEpollBufferSize = 64;        
};

#endif // OCTILLION_SSL_SERVER_HEADER
