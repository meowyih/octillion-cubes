#ifndef OCTILLION_DATABASE_HEADER
#define OCTILLION_DATABASE_HEADER

#include <cstdint>

#include "error/ocerror.hpp"
#include "world/player.hpp"

namespace octillion
{
    class Database;
}

class octillion::Database
{
public:  
    virtual std::error_code load( uint32_t pcid, Player* player ) = 0;
    virtual std::error_code save( Player* player ) = 0;
    virtual uint32_t reservedpcid() = 0;
};

#endif