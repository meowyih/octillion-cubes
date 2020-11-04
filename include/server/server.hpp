#ifndef OCTILLION_SERVER_HEADER
#define OCTILLION_SERVER_HEADER

#include <mutex>
#include <string>
#include <thread>
#include <system_error>
#include <map>
#include <list>
#include <cstdint>
#include <memory>

namespace octillion
{
    class ServerCallback;
    class Server;
}

// SslServerCallback definition
class octillion::ServerCallback
{
    private:
        const std::string tag_ = "ServerCallback";
        
    public:
        ~ServerCallback() {}
        
    public:
        // pure virtual function that handlers all incoming event
        virtual void connect( int fd ) = 0;
        
        // return 0 to close the fd due to invalid data
        virtual int recv( int fd, uint8_t* data, size_t datasize) = 0;
        virtual void disconnect( int fd ) = 0;
};

// CodeServer definition
class octillion::Server 
{
    private:
        const std::string tag_ = "Server";
        
    // singleton
    public:
        static Server& get_instance()
        {
            static Server instance;
            return instance;
        }
        
    public:        
        // start the server thread
        std::error_code start( std::string port );
        
        // raise the stop flag and wait until the server thread die
        std::error_code stop();

        // send data vid a fd. this function is thread safe
        std::error_code senddata( int fd, const void *buf, size_t len, bool closefd = false );
        std::error_code senddata( int fd, std::vector<uint8_t>& data, bool closefd = false );

        // add fd into close queue and will be closed later in core_task thread
        std::error_code requestclosefd(int fd);

        // check if server thread is still running
        bool is_running() { return is_running_; }
        
        // assign callback class, multiple callback instances is not supported
        void set_callback( ServerCallback* callback ) { callback_ = callback; }
        
        std::string getip( int fd );
        
    private:
        Server();
        ~Server();        
        
        // send data via a socket fd, should be called inside the server thread
        void closesocket( int socketfd );
        
        void core_task();
        
        std::error_code init_server_socket();
        std::error_code set_nonblocking( int fd );
        
    private: // debug usage
        static std::string get_epoll_event( uint32_t event );    
        static std::string get_errno_string();
    
    private:
        // epoll timeout
        int epoll_timeout_;        
        
        // epoll event buffer size
        int epoll_buffer_size_;
        
        ServerCallback* callback_;
        
    public:                
        // avoid accidentally copy
        Server( Server const& ) = delete;
        void operator = ( Server const& ) = delete;

    private:
        std::string port_;
        int server_fd_;
        int epoll_fd_;
        
        bool is_running_;
        bool core_thread_flag_;
        
        // std::thread*    core_thread_;
        std::unique_ptr<std::thread> core_thread_;
        
    private:
        // SSL_write data buffer
        struct DataBuffer
        {
            int fd;
            // uint8_t* data;
            // size_t datalen;
            std::shared_ptr<std::vector<uint8_t>> data;
            bool closefd;
        };
                
        std::list<DataBuffer> out_data_;
        std::mutex out_data_lock_;
        
        // client socket list
        struct Socket
        {
            int fd;
            bool writable;
            unsigned long s_addr;
        };
        std::map<int,Socket> sockets_;

        // fd that waiting for close
        std::mutex badfds_lock_;
        std::list<int> badfds_;
    
    private:
        const int kEpollTimeout = 1 * 1000;
        const int kEpollBufferSize = 64;        
};

#endif // OCTILLION_SERVER_HEADER
