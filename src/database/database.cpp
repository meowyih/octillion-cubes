
#include "database/database.hpp"

octillion::Database::Database()
{
    reservedpcid_ = 100;
}

uint32_t octillion::Database::reservedpcid()
{
    // TODO: use MySql to create a user with no data
    return reservedpcid_++;
}

