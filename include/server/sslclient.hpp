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
        virtual int recv( int id, std::error_code error, uint8_t* data, size_t datasize) = 0;
};

// SslClient definition
class octillion::SslClient 
{
    private:
        const std::string tag_ = "SslClient";
        const static long int timeout_ = 5; // connect timeout in 5 seconds
        
    // singleton
    public:
        static SslClient& get_instance()
        {
            static SslClient instance;
            return instance;
        }
        
    public:
        std::error_code write( int id, std::string hostname, std::string port, uint8_t* data, size_t datasize );
        void set_callback( SslClientCallback* callback ) { callback_ = callback; }
        void force_stop() { wait_for_stop_ = true; }
        int callback( int id, std::error_code err, uint8_t* data, size_t datasize);
        
    public:                
        // avoid accidentally copy
        SslClient( SslClient const& ) = delete;
        void operator = ( SslClient const& ) = delete;

    private:
        SslClient();
        ~SslClient(); 
        
    protected:

        void core_task(
            int id,
            std::string hostname, 
            std::string port, 
            uint8_t* data, 
            size_t datasize );

    private:
        std::map<int,std::thread*> threads_;        
        bool wait_for_stop_;
        SSL_CTX *ctx_;
        
    private:
        SslClientCallback* callback_;
};

#endif