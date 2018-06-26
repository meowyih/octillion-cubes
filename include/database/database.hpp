#ifndef OCTILLION_DATABASE_HEADER
#define OCTILLION_DATABASE_HEADER

#include <cstdint>
#include <string>

#include "error/ocerror.hpp"
#include "world/player.hpp"

namespace octillion
{
    class Database;
}

class octillion::Database
{
public:
    virtual uint32_t pcid(std::string name) = 0;
    virtual std::string hashpassword(std::string password) = 0;
    virtual std::error_code reserve(int fd, std::string name) = 0;
    virtual std::error_code create( int fd, Player* player) = 0;
    virtual std::error_code load( uint32_t pcid, Player* player ) = 0;
    virtual std::error_code save( Player* player ) = 0;
};

#endif