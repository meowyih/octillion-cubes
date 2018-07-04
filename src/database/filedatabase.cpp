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

#include "jsonw/jsonw.hpp"
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

uint32_t octillion::FileDatabase::login(std::string name, std::string password)
{
    LOG_D(tag_) << "login start, name:" << name << " pwd:" << password;

    std::string hashedpwd = hashpassword( password );    
    
    auto it = userlist_.find( name );    
    if ( it == userlist_.end() )
    {
        LOG_I(tag_) << "login failed, no username:" << name;
        return 0;
    }
    
    if ( it->second.password == hashedpwd )
    {
        LOG_I(tag_) << "login successfully, username:" << name;
        return it->second.pcid;
    }
    
    LOG_I(tag_) << "login failed, existing name:" << name << " wrong pwd:" << password;
    return 0;
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

    // set player's location
    CubePosition loc(100000, 100000, 100000);
    player->position(loc);

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
    std::wifstream wfin(filename);
    if (wfin.good())
    {
        JsonTextW json(wfin);

        if (json.valid() == false)
        {
            LOG_E(tag_) << "fatal error, player file:" << filename << " contains invalid json data";
            return OcError::E_DB_BAD_RECORD;
        }

        JsonValueW* jvalue = json.value();
        JsonObjectW* jobject = jvalue->object();

        if (jobject == NULL)
        {
            LOG_E(tag_) << "fatal error, player file:" << filename << " has no json object";
            return OcError::E_DB_BAD_RECORD;
        }

        // check if json is valid
        if (jobject->find(u8"id") == NULL || jobject->find(u8"id")->type() != JsonValueW::Type::NumberInt ||
            jobject->find(u8"username") == NULL || jobject->find(u8"username")->type() != JsonValueW::Type::String ||
            jobject->find(u8"password") == NULL || jobject->find(u8"password")->type() != JsonValueW::Type::String ||
            jobject->find(u8"gender") == NULL || jobject->find(u8"gender")->type() != JsonValueW::Type::NumberInt ||
            jobject->find(u8"cls") == NULL || jobject->find(u8"cls")->type() != JsonValueW::Type::NumberInt ||
            jobject->find(u8"con") == NULL || jobject->find(u8"con")->type() != JsonValueW::Type::NumberInt ||
            jobject->find(u8"men") == NULL || jobject->find(u8"men")->type() != JsonValueW::Type::NumberInt ||
            jobject->find(u8"luc") == NULL || jobject->find(u8"luc")->type() != JsonValueW::Type::NumberInt ||
            jobject->find(u8"cha") == NULL || jobject->find(u8"cha")->type() != JsonValueW::Type::NumberInt ||
            jobject->find(u8"loc") == NULL || jobject->find(u8"loc")->type() != JsonValueW::Type::JsonArray)
        {
            LOG_E(tag_) << "fatal error, player file:" << filename << " missing json members";
            return OcError::E_DB_BAD_RECORD;
        }

        // retrieve data
        player->id(jobject->find(u8"id")->integer());
        player->username(jobject->find(u8"username")->string());
        player->password(jobject->find(u8"password")->string());
        player->gender(jobject->find(u8"gender")->integer());
        player->cls(jobject->find(u8"cls")->integer());
        player->con(jobject->find(u8"con")->integer());
        player->men(jobject->find(u8"men")->integer());
        player->luc(jobject->find(u8"luc")->integer());
        player->cha(jobject->find(u8"cha")->integer());

        JsonArrayW* jarray = jobject->find(u8"loc")->array();

        if (jarray->size() != 3 || 
            jarray->at(0)->type() != JsonValueW::Type::NumberInt ||
            jarray->at(1)->type() != JsonValueW::Type::NumberInt ||
            jarray->at(2)->type() != JsonValueW::Type::NumberInt )
        {
            LOG_E(tag_) << "fatal error, player file:" << filename << " json has bad loc field";
            return OcError::E_DB_BAD_RECORD;
        }

        CubePosition loc(
            jarray->at(0)->integer(), 
            jarray->at(1)->integer(), 
            jarray->at(2)->integer());

        player->position(loc);

        return OcError::E_SUCCESS;
    }
    else
    {
        // user file does not exist
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

    JsonObjectW* jobject = new JsonObjectW();
    jobject->add(u8"username", player->username());
    jobject->add(u8"password", player->password());
    jobject->add(u8"id", (int)player->id());
    jobject->add(u8"gender", (int)player->gender());
    jobject->add(u8"cls", (int)player->cls());
    jobject->add(u8"con", (int)player->con());
    jobject->add(u8"men", (int)player->men());
    jobject->add(u8"luc", (int)player->luc());
    jobject->add(u8"cha", (int)player->cha());

    JsonArrayW* jarray = new JsonArrayW();
    jarray->add((int)(player->position().x()));
    jarray->add((int)(player->position().y()));
    jarray->add((int)(player->position().z()));
    jobject->add(u8"loc", jarray);

    JsonTextW jtext(jobject);

    std::ofstream pcfile(filename, std::ofstream::out | std::ofstream::trunc);
    pcfile << jtext.string();

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