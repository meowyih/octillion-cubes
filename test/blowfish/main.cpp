#include <cstring>
#include <iostream>
#include <cassert>
#include <chrono>

#include "auth/blowfish.hpp"

int main2()
{    
    int counter = 1;
    
    uint64_t time_1, time_2;
    
    // 24 bytes digest
    uint8_t digest_1[24], digest_2[24];
    
    // 16 bytes salt
    uint8_t salt_good[16+1] = "1234567890123456";
    uint8_t salt_bad[16+1] =  "123456789O123456";
    
    // password with pepper
    const char* password_good = "pepper_is_here_and_pwd_is_640222";
    const char* password_bad =  "pepper_is_here_and_pwd_is_642222";
    
    int cost = 10; // maximum 63
    int ret;
    
    // test 1: correct password
    ::blowfish_bcrypt( (void*) digest_1, password_good, std::strlen( password_good ), salt_good, cost );
    ::blowfish_bcrypt( (void*) digest_2, password_good, std::strlen( password_good ), salt_good, cost );    
    assert( std::memcmp( (const void*)digest_1, (const void*)digest_2, 24 ) == 0 );    
    std::cout << "passed" << counter++ << std::endl;

    // test 2: wrong password 
    ::blowfish_bcrypt( (void*) digest_2, password_bad, std::strlen( password_bad ), salt_good, cost );    
    assert( std::memcmp( (const void*)digest_1, (const void*)digest_2, 24 ) != 0 ); 
    std::cout << "passed" << counter++ << std::endl;
    
    // test 3: correct password with bad salt
    ::blowfish_bcrypt( (void*) digest_2, password_good, std::strlen( password_good ), salt_bad, cost );
    assert( std::memcmp( (const void*)digest_1, (const void*)digest_2, 24 ) != 0 ); 
    std::cout << "passed" << counter++ << std::endl; 

    // test 4: correct password with different cost
    ::blowfish_bcrypt( (void*) digest_2, password_good, std::strlen( password_good ), salt_good, cost - 1 );
    assert( std::memcmp( (const void*)digest_1, (const void*)digest_2, 24 ) != 0 ); 
    std::cout << "passed" << counter++ << std::endl;
    
    // test the calcuation time for each blowfish_bcrypt with different 'cost'
    for ( int i = 0; i < 64; i ++ )
    {
        time_1 = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()).count();
            
        ::blowfish_bcrypt( (void*) digest_2, password_good, std::strlen( password_good ), salt_good, i );
            
        time_2 = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()).count();
            
        std::cout << "cost " << i << " spends:" << (time_2 - time_1 ) << " milliseconds." << std::endl;
    }
    
    return 0;
}