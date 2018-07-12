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
        count = (uint_fast32_t)std::stoi(line);

        for (size_t i = 0; i < count; i ++ )
        {
            // idx file exist
            FileDatabaseListItem item;
            std::string username;
            std::getline(idxfile, username);
            std::getline(idxfile, line);
            item.pcid = (uint_fast32_t)std::stoi(line);
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

uint_fast32_t octillion::FileDatabase::login(std::string name, std::string password)
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

uint_fast32_t octillion::FileDatabase::pcid(std::string name)
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
    uint_fast32_t id = pcid(name);

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

std::error_code octillion::FileDatabase::create( int fd, Player* player, CubePosition& loc)
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
    loc.set(
        (uint_fast32_t)100000,
        (uint_fast32_t)100000,
        (uint_fast32_t)100000);

    item.pcid = player->id();
    item.password = player->password();
    userlist_[player->username()] = item;
    flushidx();

    // also, save user data into personal record
    return save(player);
}
      
std::error_code octillion::FileDatabase::load( uint_fast32_t pcid, Player* player, CubePosition& loc)
{    
    if ( player == NULL )
    {
        return OcError::E_FATAL;
    }
    
    std::string filename = pcfilename(pcid);

    // read file   
    std::ifstream fin(filename);
    if (fin.good())
    {
        JsonW json(fin);

        if (json.valid() == false || json.type() != JsonW::OBJECT )
        {
            LOG_E(tag_) << "fatal error, player file:" << filename << " contains invalid json data";
            return OcError::E_DB_BAD_RECORD;
        }

        // check if json is valid
        if (json.get(u8"id") == NULL || json.get(u8"id")->type() != JsonW::INTEGER ||
            json.get(u8"username") == NULL || json.get(u8"username")->type() != JsonW::STRING ||
            json.get(u8"password") == NULL || json.get(u8"password")->type() != JsonW::STRING ||
            json.get(u8"gender") == NULL || json.get(u8"gender")->type() != JsonW::INTEGER ||
            json.get(u8"cls") == NULL || json.get(u8"cls")->type() != JsonW::INTEGER ||
            json.get(u8"con") == NULL || json.get(u8"con")->type() != JsonW::INTEGER ||
            json.get(u8"men") == NULL || json.get(u8"men")->type() != JsonW::INTEGER ||
            json.get(u8"luc") == NULL || json.get(u8"luc")->type() != JsonW::INTEGER ||
            json.get(u8"cha") == NULL || json.get(u8"cha")->type() != JsonW::INTEGER ||
            json.get(u8"loc") == NULL || json.get(u8"loc")->type() != JsonW::ARRAY)
        {
            LOG_E(tag_) << "fatal error, player file:" << filename << " missing json members";
            return OcError::E_DB_BAD_RECORD;
        }

        // retrieve data
        player->id((uint_fast32_t)(json.get(u8"id")->integer()));
        player->username(json.get(u8"username")->str());
        player->password(json.get(u8"password")->str());
        player->gender((uint_fast32_t)json.get(u8"gender")->integer());
        player->cls((uint_fast32_t)json.get(u8"cls")->integer());
        player->con((uint_fast32_t)json.get(u8"con")->integer());
        player->men((uint_fast32_t)json.get(u8"men")->integer());
        player->luc((uint_fast32_t)json.get(u8"luc")->integer());
        player->cha((uint_fast32_t)json.get(u8"cha")->integer());

        JsonW* jloc = json.get(u8"loc");

        if (jloc->size() != 3 ||
            jloc->get(0)->type() != JsonW::INTEGER ||
            jloc->get(1)->type() != JsonW::INTEGER ||
            jloc->get(2)->type() != JsonW::INTEGER)
        {
            LOG_E(tag_) << "fatal error, player file:" << filename << " json has bad loc field";
            return OcError::E_DB_BAD_RECORD;
        }

        loc.set(
            (uint_fast32_t) jloc->get(0)->integer(),
            (uint_fast32_t) jloc->get(1)->integer(),
            (uint_fast32_t) jloc->get(2)->integer());

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

    JsonW* jobject = new JsonW();
    jobject->add(u8"username", player->username());
    jobject->add(u8"password", player->password());
    jobject->add(u8"id", (int)player->id());
    jobject->add(u8"gender", (int)player->gender());
    jobject->add(u8"cls", (int)player->cls());
    jobject->add(u8"con", (int)player->con());
    jobject->add(u8"men", (int)player->men());
    jobject->add(u8"luc", (int)player->luc());
    jobject->add(u8"cha", (int)player->cha());

    JsonW* jloc = new JsonW();
    jloc->add((long long)(player->cube()->loc().x()));
    jloc->add((long long)(player->cube()->loc().y()));
    jloc->add((long long)(player->cube()->loc().z()));
    jobject->add(u8"loc", jloc);

    std::ofstream pcfile(filename, std::ofstream::out | std::ofstream::trunc);
    pcfile << (jobject->text(false));

    delete jobject;

    return OcError::E_SUCCESS;
}

std::string octillion::FileDatabase::pcfilename( uint_fast32_t pcid )
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