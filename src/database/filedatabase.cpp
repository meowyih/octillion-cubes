#include <fstream>
#include <sstream>
#include <string>

#ifdef WIN32
// WIN32 create directory
#include <Windows.h>
#include <locale>
#include <codecvt>
#else
// POSIX create directory
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "error/macrolog.hpp"
#include "error/ocerror.hpp"
#include "database/filedatabase.hpp"

const std::string octillion::FileDatabase::idxfile_ = "idxdb";
const std::string octillion::FileDatabase::pplprefix_ = "ppl";

octillion::FileDatabase::FileDatabase()
{
    LOG_D(tag_) << "Constructor";
}

octillion::FileDatabase::~FileDatabase()
{
    LOG_D(tag_) << "Destructor";
    flushidx();
}

void octillion::FileDatabase::init(std::string directory)
{
    LOG_D(tag_) << "init enter";
    // create directory if not exist, Linux POSIX
    directory_ = directory;

    if (directory_.length() >= 1)
    {
        struct stat st = { 0 };

        if (stat(directory_.c_str(), &st) == -1)
        {
#ifdef WIN32 // create directory if needed
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring wdir = converter.from_bytes(directory_.c_str());
            CreateDirectory(wdir.c_str(), NULL);
#else
            mkdir(directory_.c_str(), 0700);
#endif
        }

#ifdef WIN32 // append directory seperator
        if (directory_.at(directory_.length() - 1) != '\\')
        {
            directory_.append("\\");
        }
#else
        if (directory_.at(directory_.length() - 1) != '/')
        {
            directory_.append("/");
        }
#endif
    }

    maxpcid_ = 100;

    // read file      
    std::ifstream idxfile(idxfilename());
    if (idxfile.good())
    {
        size_t count;
        std::string line;
        std::getline(idxfile, line);
        count = (uint32_t)std::stoi(line);

        for (size_t i = 0; i < count; i ++ )
        {
            // idx file exist
            FileDatabaseListItem item;
            std::string username;
            std::getline(idxfile, username);
            std::getline(idxfile, line);
            item.pcid = (uint32_t)std::stoi(line);
            std::getline(idxfile, line);
            item.password = line;

            // check if duplicate username
            if (userlist_.find(username) != userlist_.end())
            {
                // fatal error
                LOG_E(tag_) << "filedatabase find duplicate username in idx file, name:" << username;
            }
            else
            {
                userlist_[username] = item;

                if (item.pcid > maxpcid_)
                {
                    maxpcid_ = item.pcid;
                }
            }
        }

        idxfile.close();
    }
    else
    {
        flushidx();
    }

    LOG_D(tag_) << "init leave, directory_:" << directory_;
}

// TODO: use openssl library to hash password
std::string octillion::FileDatabase::hashpassword(std::string password)
{
    return password;
}

std::error_code octillion::FileDatabase::flushidx()
{
    std::ofstream idxfile( idxfilename(), std::ofstream::out | std::ofstream::trunc );

    idxfile << userlist_.size() << std::endl;

    for (const auto& it : userlist_) 
    {
        idxfile << it.first << std::endl;
        idxfile << it.second.pcid << std::endl;
        idxfile << it.second.password << std::endl;
    }

    idxfile.close();
    
    return OcError::E_SUCCESS;
}

uint32_t octillion::FileDatabase::pcid(std::string name)
{
    auto it = userlist_.find(name);

    if (it == userlist_.end())
    {
        return 0;
    }
    else
    {
        return it->second.pcid;
    }
}

std::error_code octillion::FileDatabase::reserve(int fd, std::string name)
{
    uint32_t id = pcid(name);

    // name already exists in database, reserve failed
    if (id != 0)
    {
        return OcError::E_DB_DUPLICATE_USERNAME;
    }

    // if fd exists in reserve list, delete the previous one
    auto it = reservelist_.find(fd);
    if (it != reservelist_.end())
    {
        reservelist_.erase(it);
    }

    // check if reservelist_ contains same name as 
    for(auto& it : reservelist_)
    {
        if (it.second == name)
        {
            // same name already exist
            return OcError::E_DB_DUPLICATE_USERNAME;
        }
    }

    // make a reservation
    reservelist_[fd] = name;

    return OcError::E_SUCCESS;
}

std::error_code octillion::FileDatabase::create( int fd, Player* player)
{
    FileDatabaseListItem item;
    std::string username = player->username();

    // check if player user name already exist
    if (pcid(username) != 0 )
    {
        return OcError::E_DB_DUPLICATE_USERNAME;
    }

    // check if user name already reserved by other player
    auto it = reservelist_.find(fd);
    if (it != reservelist_.end())
    {
        // already reserved a name
        reservelist_.erase(it);
    }
    
    for(auto it : reservelist_)
    {
        if (it.second == username )
        {
            return OcError::E_DB_DUPLICATE_USERNAME;
        }
    }

    // assign pcid, insert player information into userlist and save to file
    maxpcid_++;
    player->id(maxpcid_);

    item.pcid = player->id();
    item.password = player->password();
    userlist_[player->username()] = item;
    flushidx();

    // also, save user data into personal record
    return save(player);
}
      
std::error_code octillion::FileDatabase::load( uint32_t pcid, Player* player )
{    
    if ( player == NULL )
    {
        return OcError::E_FATAL;
    }
    
    std::string filename = pcfilename(pcid);

    // read file   
    std::ifstream idxfile(filename);
    if (idxfile.good())
    {
        // user file does not exist
        std::string line;

        std::getline(idxfile, line);
        player->username(line);

        std::getline(idxfile, line);
        player->password(line);

        std::getline(idxfile, line);
        player->id((uint32_t)std::stoi(line));

        std::getline(idxfile, line);
        player->id((uint32_t)std::stoi(line));

        std::getline(idxfile, line);
        player->cls((uint32_t)std::stoi(line));

        std::getline(idxfile, line);
        player->gender((uint32_t)std::stoi(line));

        std::getline(idxfile, line);
        player->con((uint32_t)std::stoi(line));

        std::getline(idxfile, line);
        player->men((uint32_t)std::stoi(line));

        std::getline(idxfile, line);
        player->luc((uint32_t)std::stoi(line));

        std::getline(idxfile, line);
        player->cha((uint32_t)std::stoi(line));

        idxfile.close();
        return OcError::E_SUCCESS;
    }
    else
    {
        // user file does not exist
        idxfile.close();
        return OcError::E_DB_NO_RECORD;
    }
}

std::error_code octillion::FileDatabase::save( Player* player )
{    
    if (player == NULL)
    {
        return OcError::E_FATAL;
    }

    std::string filename = pcfilename(player->id() );

    std::ofstream pcfile(filename, std::ofstream::out | std::ofstream::trunc);

    pcfile << player->username() << std::endl
        << player->password() << std::endl;

    pcfile << player->id() << std::endl
        << player->cls() << std::endl
        << player->gender() << std::endl
        << player->con() << std::endl
        << player->men() << std::endl
        << player->luc() << std::endl
        << player->cha() << std::endl;

    return OcError::E_SUCCESS;
}

std::string octillion::FileDatabase::pcfilename( uint32_t pcid )
{
    std::ostringstream stringStream;    
    stringStream << directory_ << pplprefix_ << pcid;
    return stringStream.str();
}

std::string octillion::FileDatabase::idxfilename()
{
    std::ostringstream stringStream;
    stringStream << directory_ << idxfile_;   
    return stringStream.str();
}