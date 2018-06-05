#ifndef OCTILLION_FDATABASE_HEADER
#define OCTILLION_FDATABASE_HEADER

#include <string>
#include <system_error>

#include "database/database.hpp"

namespace octillion
{
    class FileDatabase;
}

class octillion::FileDatabase : public octillion::Database
{
private:
    const std::string tag_ = "FileDatabase";

public:
    FileDatabase( std::string directory );
    ~FileDatabase();
    
    virtual std::error_code load( uint32_t pcid, Player* player ) override;
    virtual std::error_code save( Player* player ) override;
    virtual uint32_t reservedpcid() override;    
    
private:
    const static std::string idxfile_;
    const static std::string pplprefix_;
    
    std::string idxfilename();
    std::string pcfilename( uint32_t pcid );
    std::string directory_;
    
    std::error_code flushidx();
    uint32_t reservedpcid_;
};

#endif