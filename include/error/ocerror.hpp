#ifndef OC_ERROR_HEADER
#define OC_ERROR_HEADER

#include <system_error>

// inject error code into std:error
enum class OcError
{
    E_SUCCESS = 0,

    // core server
    E_SERVER_BUSY = 10,

    E_SYS_GETADDRINFO = 20,
    E_SYS_BIND = 30,
    E_SYS_FCNTL = 40,
    E_SYS_LISTEN = 50,
    E_SYS_EPOLL_CREATE = 60,
    E_SYS_EPOLL_CTL = 70,
    E_SYS_SEND_AGAIN = 80,
    E_SYS_SEND_PARTIAL = 90,
    E_SYS_SEND_WRONG_THREAD = 100,
    E_SYS_SEND = 110,
    E_SYS_OPENSSL = 120,

    E_DB_NO_RECORD = 200,
    E_DB_DUPLICATE_USERNAME = 201,
    E_DB_BAD_RECORD = 202,
    
    E_PROTOCOL_FD_NO_CONNECT = 300,
    E_PROTOCOL_FD_DUPLICATE_CONNECT = 301,
	E_PROTOCOL_FD_DUPLICATE_LOGIN = 302,
    E_PROTOCOL_FD_LOGOUT = 303,

    E_WORLD_FREEZED = 400,
    E_WORLD_BAD_CUBE_POSITION = 401,
    
    E_FATAL = 999,    
};

namespace std
{
    template<> struct is_error_code_enum<OcError> : true_type {};
}
 
std::error_code make_error_code( OcError );

#endif // OC_ERROR_HEADER