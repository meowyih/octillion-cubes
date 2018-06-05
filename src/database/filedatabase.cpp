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

octillion::FileDatabase::FileDatabase( std::string directory )
{
    LOG_D(tag_) << "Constructor enter";
    // create directory if not exist, Linux POSIX
    directory_ = directory;
    
    if ( directory_.length() >= 1 )
    {
        struct stat st = {0};

        if (stat( directory_.c_str(), &st) == -1) 
        {
#ifdef WIN32
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring wdir = converter.from_bytes(directory_.c_str());
            CreateDirectory(wdir.c_str(), NULL);        
            
            if ( directory_.at( directory_.length() - 1) != '\\')
            {
                directory_.append( "\\" );
            }
#else
            mkdir( directory_.c_str(), 0700);
        
            if ( directory_.at( directory_.length() - 1) != '/')
            {
                directory_.append( "/" );
            }
#endif
        }
        

    }
    
    // read file   
    std::ifstream idxfile( idxfilename() );
    if ( idxfile.good() )
    {
        // idx file exist
        std::string line;
        std::getline(idxfile, line );
        reservedpcid_ = (uint32_t) std::stoi( line );
        idxfile.close();
    }
    else
    {
        reservedpcid_ = 1000;
        flushidx();
    }

    LOG_D(tag_) << "Constructor leave, directory_:" << directory_;
}

octillion::FileDatabase::~FileDatabase()
{
    LOG_D(tag_) << "Destructor";
    flushidx();
}

std::error_code octillion::FileDatabase::flushidx()
{
    std::ofstream idxfile( idxfilename(), std::ofstream::out | std::ofstream::trunc );
    
    idxfile << reservedpcid_;
    idxfile.close();
    
    return OcError::E_SUCCESS;
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

    pcfile << player->id() << std::endl
        << player->cls() << std::endl
        << player->gender() << std::endl
        << player->con() << std::endl
        << player->men() << std::endl
        << player->luc() << std::endl
        << player->cha() << std::endl;

    return OcError::E_SUCCESS;
}

uint32_t octillion::FileDatabase::reservedpcid()
{
    reservedpcid_++;
    flushidx();
    return reservedpcid_;
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