#ifndef OCTILLION_FDATABASE_HEADER
#define OCTILLION_FDATABASE_HEADER

#include <string>
#include <system_error>
#include <map>
#include <mutex>

#include "database/database.hpp"

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

namespace octillion
{
    class FileDatabaseListItem;
    class FileDatabase;
}

class octillion::FileDatabaseListItem
{
public:
    uint_fast32_t pcid;
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
    
    virtual uint_fast32_t pcid(std::string name) override; 
    virtual uint_fast32_t login(std::string name, std::string password) override;
    virtual std::string hashpassword(std::string password) override;
    virtual std::error_code reserve(int fd, std::string name) override;
    virtual std::error_code create( int fd, Player* player, CubePosition& loc, CubePosition& loc_reborn) override;
    virtual std::error_code load( uint_fast32_t pcid, Player* player, CubePosition& loc, CubePosition& loc_reborn) override;
    virtual std::error_code save( Player* player ) override;
    
private:
    const static std::string idxfile_;
    const static std::string pplprefix_;
    
    std::string idxfilename();
    std::string pcfilename( uint_fast32_t pcid );
    std::string directory_;
    
    std::error_code flushidx();
    uint_fast32_t maxpcid_;

    std::map<std::string, FileDatabaseListItem> userlist_;
    std::map<int, std::string> reservelist_;

#ifdef MEMORY_DEBUG
public:
    static void* operator new(size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

    static void* operator new[](size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

        static void operator delete(void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }

    static void operator delete[](void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }
#endif
};

#endif