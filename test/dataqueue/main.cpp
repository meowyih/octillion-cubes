
#include <iostream>
#include <vector>

#include "server/dataqueue.hpp"
#include "error/ocerror.hpp"

int main()
{
    std::vector<uint8_t> raw_data( 1024 );
    std::vector<uint8_t> packet_data( raw_data.size() + sizeof( uint32_t ));
    std::vector<uint8_t> buffer;
    octillion::DataQueue dqueue;
    uint32_t size, dqsize;
    int fd;
    
    for ( int i = 0; i < raw_data.size(); i ++ )
    {
        raw_data[i] = ( i % 0xFF );
    }
    
    size = raw_data.size();
    dqsize = octillion::DataQueue::write_uint32( size );
    
    std::memcpy( (void*) packet_data.data(), (void*) &dqsize, sizeof( uint32_t ));
    std::memcpy( (void*) ( packet_data.data() + sizeof( uint32_t )), (void*) raw_data.data(), raw_data.size() );

    dqueue.feed( 1, packet_data.data(), packet_data.size() );
    dqueue.feed( 2, packet_data.data(), packet_data.size() );
    dqueue.feed( 3, packet_data.data(), packet_data.size() );
    dqueue.feed( 4, packet_data.data(), packet_data.size() );
    
    if ( dqueue.size() != 4 )
    {
        std::cout << "failed 001" << std::endl;
        return -1;
    }
    
    for ( int i = 1; i <= 4; i ++ )
    {
        size_t old_size, new_size;
        
        old_size = dqueue.size();
        
        buffer.resize( dqueue.peek() );
        
        if ( dqueue.peek() != raw_data.size() )
        {
            std::cout << "failed 002" << std::endl;
            return -1;
        }

        if ( OcError::E_FATAL == dqueue.pop( fd, buffer.data(), buffer.size() ))
        {
            std::cout << "failed 003" << std::endl;
            return -1;
        }
        
        if ( fd != i )
        {
            std::cout << "failed 004" << std::endl;
            return -1;
        }
        
        for ( int j = 0; j < buffer.size(); j ++ )
        {
            if ( buffer.at(j) != ( j % 0xFF ))
            {
                std::cout << "failed 005" << std::endl;
                return -1;
            }
        }
        
        new_size = dqueue.size();
        
        if ( new_size != old_size - 1 )
        {
            std::cout << "failed 006" << std::endl;
            return -1;
        }
    }
    
    for ( int i = 0; i < packet_data.size(); i ++ )
    {
        dqueue.feed( 1, packet_data.data() + i, 1 );
        dqueue.feed( 2, packet_data.data() + i, 1 );
        dqueue.feed( 3, packet_data.data() + i, 1 );
        dqueue.feed( 4, packet_data.data() + i, 1 );
    }
    
    if ( dqueue.size() != 4 )
    {
        std::cout << "failed 001" << std::endl;
        return -1;
    }

    for ( int i = 1; i <= 4; i ++ )
    {
        size_t old_size, new_size;
        
        old_size = dqueue.size();
        
        buffer.resize( dqueue.peek() );
        
        if ( dqueue.peek() != raw_data.size() )
        {
            std::cout << "failed 002" << std::endl;
            return -1;
        }

        if ( OcError::E_FATAL == dqueue.pop( fd, buffer.data(), buffer.size() ))
        {
            std::cout << "failed 003" << std::endl;
            return -1;
        }
        
        if ( fd != i )
        {
            std::cout << "failed 004" << std::endl;
            return -1;
        }
        
        for ( int j = 0; j < buffer.size(); j ++ )
        {
            if ( buffer.at(j) != ( j % 0xFF ))
            {
                std::cout << "failed 005" << std::endl;
                return -1;
            }
        }
        
        new_size = dqueue.size();
        
        if ( new_size != old_size - 1 )
        {
            std::cout << "failed 006" << std::endl;
            return -1;
        }
    }
    
    dqueue.feed( 1, packet_data.data(), packet_data.size() );
    dqueue.feed( 2, packet_data.data(), packet_data.size() );
    dqueue.feed( 3, packet_data.data(), packet_data.size() );
    dqueue.feed( 4, packet_data.data(), packet_data.size() );
    
    dqueue.remove( 3 );
    
    if ( dqueue.size () != 3 )
    {
        std::cout << "failed 007" << std::endl;
        return -1;
    }
    
    for ( int i = 1; i <= 4; i ++ )
    {
        if ( i == 3 )
        {
            continue;
        }
        
        size_t old_size, new_size;
        
        old_size = dqueue.size();
        
        buffer.resize( dqueue.peek() );
        
        if ( dqueue.peek() != raw_data.size() )
        {
            std::cout << "failed 008" << std::endl;
            return -1;
        }

        if ( OcError::E_FATAL == dqueue.pop( fd, buffer.data(), buffer.size() ))
        {
            std::cout << "failed 009" << std::endl;
            return -1;
        }
        
        if ( fd != i )
        {
            std::cout << "failed 010" << std::endl;
            return -1;
        }
        
        for ( int j = 0; j < buffer.size(); j ++ )
        {
            if ( buffer.at(j) != ( j % 0xFF ))
            {
                std::cout << "failed 01" << std::endl;
                return -1;
            }
        }
        
        new_size = dqueue.size();
        
        if ( new_size != old_size - 1 )
        {
            std::cout << "failed 012" << std::endl;
            return -1;
        }
    }
    
    std::cout << "Passed" << std::endl;
        
    return 0;
}