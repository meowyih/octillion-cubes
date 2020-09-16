
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>

#include <signal.h>

// #include "t001.hpp"
// #include "t002.hpp"
// #include "t003.hpp"
// #include "t004.hpp"
// #include "t005.hpp"
#include "error/ocerror.hpp"
#include "server/sslserver.hpp"
#include "server/sslclient.hpp"
// #include "server/dataqueue.hpp"
// #include "world/world.hpp"
// #include "server/rawprocessor.hpp"
#include "error/macrolog.hpp"

volatile sig_atomic_t flag = 0;

void my_function(int sig)
{
    std::cout << "stopping the server..." << std::endl;
    flag = 1;
}

namespace octillion
{
    class SslServerCb;    
}

class octillion::SslServerCb : public octillion::SslServerCallback
{
    public:
        // virtual function from SslServerCallback that handlers all incoming events
        virtual void connect( int fd ) override
        {
            std::cout << "connect:" << fd << std::endl;
        }
        
        virtual int recv( int fd, uint8_t* data, size_t datasize) override
        {
            std::cout << "recv:" << datasize << std::endl;
            return datasize;
        }
        
        virtual void disconnect( int fd ) override
        {
            std::cout << "disconnect:" << fd << std::endl;
        }
};

int main ()
{    
    std::error_code err;
    
    LOG_I() << "main start";

    signal(SIGINT, my_function);

#if 0   
    g_test005.test();
    
    while ( flag == 0 ) 
    {
    }
    
    g_test005.stop();
#endif    
#if 1
    // don't forget to change iptables
    // iptables -A ufw-user-input -p tcp -m tcp --dport 8888 -j ACCEPT
    // iptables -A ufw-user-output -p tcp -m tcp --dport 8888 -j ACCEPT
    flag = 0;
    octillion::SslServerCb *callback = new octillion::SslServerCb();
    uint64_t lastms = (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();    
    
    octillion::SslServer::get_instance().set_callback( callback );
    err = octillion::SslServer::get_instance().start( "8888", "../cert/cert.key", "../cert/cert.pem" );
    
    std::cout << "enter the loop" << std::endl;
    while( 1 )
    {
        uint64_t nowms = (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();
        
        if ( nowms - lastms > 1000 )
        {
            lastms = nowms;
        }
        
        if ( flag == 1 )
        {
            std::cout << "break" << std::endl;
            break;
        }
    }
    std::cout << "leave the loop" << std::endl;
    octillion::SslServer::get_instance().set_callback( NULL );
    delete callback;
    if ( err != OcError::E_SUCCESS )
    {
        std::cout << err << std::endl;
        return 1;
    }
#endif
    // octillion::T001* test001 = new octillion::T001();    
    // octillion::SslServer::get_instance().set_callback( test001 );
    // test001->testSingleConn();
    // test001->testSingleConn();
    // test001->testSingleConn();
        
    // octillion::T002* test002 = new octillion::T002();    
    // octillion::SslServer::get_instance().set_callback( test002 );
    
    // test002->testMultipleConn();
    // test002->testMultipleConn();
    
    // octillion::T003* test003 = new octillion::T003();    
    // octillion::SslServer::get_instance().set_callback( test003 );
    // test003->test();
#if 0    
    octillion::T004 test004;
    test004.test();

    flag = 0;
    uint64_t lastms = (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();
    
    std::cout << "enter the loop" << std::endl;
    while( 1 )
    {
        uint64_t nowms = (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();
        
        if ( nowms - lastms > 1000 )
        {
            lastms = nowms;
        }
        
        if ( flag == 1 )
        {
            std::cout << "break" << std::endl;
            break;
        }
    }
    std::cout << "leave the loop" << std::endl;
    
    if ( octillion::SslServer::get_instance().is_running() )
    {
        std::cout << "octillion::SslServer::get_instance().stop()" << std::endl;
        octillion::SslServer::get_instance().stop();
    }
    else
    {
        std::cout << "server is not running" << std::endl;
    }
#endif
    // delete test001;
    // delete test002;
    // delete test003;

    return 0;
}
