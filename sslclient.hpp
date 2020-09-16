#ifndef OCTILLION_SSL_CLIENT_HEADER
#define OCTILLION_SSL_CLIENT_HEADER

#include <mutex>
#include <string>
#include <thread>
#include <system_error>
#include <map>
#include <list>
#include <cstdint>

#include <openssl/ssl.h>

//
// RESTFUL openssl client 
//

namespace octillion
{
    class SslClientCallback;
    class SslClient;
}

// SslClientCallback definition
class octillion::SslClientCallback
{
    private:
        const std::string tag_ = "SslClientCallback";
     
    public:
        ~SslClientCallback() {}
        
    public:       
        virtual int recv( uint8_t* data, size_t datasize) = 0;
};

// SslClient definition
class octillion::SslClient 
{
    private:
        const std::string tag_ = "SslClient";
        
    // singleton
    public:
        static SslClient& get_instance()
        {
            static SslClient instance;
            return instance;
        }
        
    public:
        std::error_code write( std::string hostname, std::string port, uint8_t* data, size_t datasize );
        void set_callback( SslClientCallback* callback ) { callback_ = callback; }
        bool is_running();
        void force_stop() { wait_for_stop_ = true; }
        
    public:                
        // avoid accidentally copy
        SslClient( SslClient const& ) = delete;
        void operator = ( SslClient const& ) = delete;

    private:
        SslClient();
        ~SslClient(); 
        
    private:
        void core_task();

    private:
        std::thread* core_thread_;
        std::string  hostname_;
        std::string  port_;
        
        bool wait_for_stop_;
        bool is_running_;
        bool core_thread_flag_;
        
        SSL_CTX *ctx_;
        
        uint8_t* data_; 
        size_t datasize_; 

    private:
        SslClientCallback* callback_;
};

#endif