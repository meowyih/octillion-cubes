#ifndef OCTILLION_FDATABASE_HEADER
#define OCTILLION_FDATABASE_HEADER

#include <string>
#include <system_error>
#include <map>
#include <mutex>

#include "database/database.hpp"

namespace octillion
{
    class FileDatabaseListItem;
    class FileDatabase;
}

class octillion::FileDatabaseListItem
{
public:
    uint32_t pcid;
    std::string password;
};

class octillion::FileDatabase : public octillion::Database
{
private:
    const std::string tag_ = "FileDatabase";

public:
    FileDatabase();
    ~FileDatabase();

    void init(std::string directory);
    
    virtual uint32_t pcid(std::string name) override; 
    virtual std::string hashpassword(std::string password) override;
    virtual std::error_code reserve(int fd, std::string name) override;
    virtual std::error_code create( int fd, Player* player) override;
    virtual std::error_code load( uint32_t pcid, Player* player ) override;
    virtual std::error_code save( Player* player ) override;
    
private:
    const static std::string idxfile_;
    const static std::string pplprefix_;
    
    std::string idxfilename();
    std::string pcfilename( uint32_t pcid );
    std::string directory_;
    
    std::error_code flushidx();
    uint32_t maxpcid_;

    std::map<std::string, FileDatabaseListItem> userlist_;
    std::map<int, std::string> reservelist_;
};

#endif