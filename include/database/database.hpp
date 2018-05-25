#ifndef OCTILLION_DATABASE_HEADER
#define OCTILLION_DATABASE_HEADER

#include <cstdint>

namespace octillion
{
    class Database;
}

class octillion::Database
{
// singleton
public:
    static Database& get_instance()
    {
        static Database instance;
        return instance;
    }

public:                
    // avoid accidentally copy
    Database( Database const& ) = delete;
    void operator = ( Database const& ) = delete;    

public:
    Database();
    
public:
    uint32_t reservedpcid();
    
// for small amount test, we load all player into memory.
private:
    uint32_t reservedpcid_;
};

#endif