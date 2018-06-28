
#include <string>
#include <cstring>

// ntohl / htonl 
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "error/macrolog.hpp"
#include "world/command.hpp"
#include "database/database.hpp"
#include "jsonw/jsonw.hpp"

octillion::Command::Command( uint32_t fd, uint8_t* data, size_t datasize )
{
    uint8_t* buf = data;
    size_t minsize = sizeof(uint32_t); // data should at least have 1 cmd
    size_t remaindata = datasize;
    int uiparm;
    std::string strparm;
    JsonValueW* jsonvalue;

    LOG_D( tag_ ) << "constructor, datasize:" << datasize;
    fd_ = fd;
    valid_ = false;
    cmd_ = UNKNOWN;

    json_ = new JsonTextW((const char*)data, datasize);

    // valid command must be a valid json text
    if (!json_->valid())
    {
        LOG_E(tag_) << "cons, json invalid";
        return;
    }

    // valid command must contains one and only one json object
    if (json_->value()->type() != JsonValueW::Type::JsonObject)
    {
        LOG_E(tag_) << "cons, json is no an object: " << json_->string();
        return;
    }

    // valid command must contains "cmd" name-value pair as integer
    JsonObjectW* object = json_->value()->object();
    jsonvalue = object->find(u8"cmd");
    if ( jsonvalue == NULL || jsonvalue->type() != JsonValueW::Type::NumberInt )
    {
        LOG_E(tag_) << "cons, json's object has no cmd: " << json_->string();
        return;
    }

    cmd_ = jsonvalue->integer();
    
    switch( cmd_ )
    {
    case VALIDATE_USERNAME:
        jsonvalue = object->find(u8"s1");
        if (jsonvalue == NULL || jsonvalue->type() != JsonValueW::Type::String)
        {
            LOG_E(tag_) << "cons, cmd VALIDATE_USERNAME has no s1" << json_->string();
            return;
        }

        strparm = jsonvalue->string();

        if (strparm.length() < 5 )
        {
            LOG_E(tag_) << "cons, cmd VALIDATE_USERNAME contains str that too short" << json_->string();
            return;
        }

        strparms_.push_back(jsonvalue->string());
        valid_ = true;
        break;

    case CONFIRM_USER:
        // username
        jsonvalue = object->find(u8"s1");
        
        if (jsonvalue == NULL || jsonvalue->type() != JsonValueW::Type::String)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER has no s1" << json_->string();
            return;
        }

        strparm = jsonvalue->string();

        if (strparm.length() < 5)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER contains s1 that too short" << json_->string();
            return;
        }

        strparms_.push_back(jsonvalue->string());

        // password
        jsonvalue = object->find(u8"s2");

        if (jsonvalue == NULL || jsonvalue->type() != JsonValueW::Type::String)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER has no s2" << json_->string();
            return;
        }

        strparm = jsonvalue->string();

        if (strparm.length() < 5)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER contains s2 that too short" << json_->string();
            return;
        }

        strparms_.push_back(jsonvalue->string());

        // gender
        jsonvalue = object->find(u8"i1");
        

        if (jsonvalue == NULL || jsonvalue->type() != JsonValueW::Type::NumberInt)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER has no i1" << json_->string();
            return;
        }

        uiparm = jsonvalue->integer();
        if (uiparm != Player::GENDER_FEMALE && uiparm != Player::GENDER_MALE && uiparm != Player::GENDER_NEUTRAL)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER contain invalid gender:" << uiparm;
            return;
        }

        uiparms_.push_back(uiparm);

        // class
        jsonvalue = object->find(u8"i2");

        if (jsonvalue == NULL || jsonvalue->type() != JsonValueW::Type::NumberInt)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER has no i2" << json_->string();
            return;
        }

        uiparm = jsonvalue->integer();

        if (uiparm != Player::CLS_BELIEVER && uiparm != Player::CLS_SKILLER)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER contain invalid cls:" << uiparm;
            return;
        }

        uiparms_.push_back(uiparm);
        valid_ = true;
        break;
    case LOGIN:
        // username
        jsonvalue = object->find(u8"s1");
        
        if (jsonvalue == NULL || jsonvalue->type() != JsonValueW::Type::String)
        {
            LOG_E(tag_) << "cons, cmd LOGIN has no s1" << json_->string();
            return;
        }

        strparm = jsonvalue->string();

        if (strparm.length() < 5)
        {
            LOG_E(tag_) << "cons, cmd LOGIN contains s1 that too short" << json_->string();
            return;
        }

        strparms_.push_back(jsonvalue->string());

        // password
        jsonvalue = object->find(u8"s2");

        if (jsonvalue == NULL || jsonvalue->type() != JsonValueW::Type::String)
        {
            LOG_E(tag_) << "cons, cmd LOGIN has no s2" << json_->string();
            return;
        }

        strparm = jsonvalue->string();

        if (strparm.length() < 5)
        {
            LOG_E(tag_) << "cons, cmd LOGIN contains s2 that too short" << json_->string();
            return;
        }

        strparms_.push_back(jsonvalue->string());
        valid_ = true;
        break;        
    default:
        // unknown cmd
        return;
    }
}

octillion::Command::~Command()
{
    if (json_ != NULL)
    {
        delete json_;
    }
}