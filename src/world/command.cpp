
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

octillion::Command::Command(int fd, int cmd)
{
    switch (cmd)
    {
    case CONNECT:
    case DISCONNECT:
        valid_ = true;
        break;
    default:
        valid_ = false;
    }

    fd_ = fd;
    cmd_ = cmd;
}

octillion::Command::Command( int fd, uint8_t* data, size_t datasize )
{
    uint8_t* buf = data;
    size_t minsize = sizeof(uint_fast32_t); // data should at least have 1 cmd
    size_t remaindata = datasize;
    uint_fast32_t uiparm;
    std::string strparm;

    LOG_D( tag_ ) << "constructor, datasize:" << datasize;
    fd_ = fd;
    valid_ = false;
    cmd_ = UNKNOWN;

    json_.json((const char*)data, datasize);

    // valid command must be a valid json text
    if ( json_.valid() == false )
    {
        LOG_E(tag_) << "cons, json invalid";
        return;
    }

    // valid command must contains one and only one json object
    if (json_.type() != JsonW::OBJECT)
    {
        LOG_E(tag_) << "cons, json is no an object: " << json_;
        return;
    }

    // valid command must contains "cmd" name-value pair as integer
    JsonW* jcmd = json_.get(u8"cmd");
    JsonW* jvalue;
    if (jcmd == NULL || jcmd->type() != JsonW::INTEGER )
    {
        LOG_E(tag_) << "cons, json's object has no cmd: " << json_;
        return;
    }

    cmd_ = (int)(jcmd->integer());
    
    switch( cmd_ )
    {
    case VALIDATE_USERNAME:
        jvalue = json_.get(u8"s1");
        if (jvalue == NULL || jvalue->type() != JsonW::STRING)
        {
            LOG_E(tag_) << "cons, cmd VALIDATE_USERNAME has no s1" << json_;
            return;
        }

        strparm = jvalue->str();

        if (strparm.length() < 5 )
        {
            LOG_E(tag_) << "cons, cmd VALIDATE_USERNAME contains str that too short" << json_;
            return;
        }

        strparms_.push_back(strparm);
        valid_ = true;
        break;

    case CONFIRM_USER:
        // username
        jvalue = json_.get(u8"s1");
        if (jvalue == NULL || jvalue->type() != JsonW::STRING)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER has no s1" << json_;
            return;
        }

        strparm = jvalue->str();

        if (strparm.length() < 5)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER contains s1 that too short" << json_;
            return;
        }

        strparms_.push_back(strparm);

        // password
        jvalue = json_.get(u8"s2");
        if (jvalue == NULL || jvalue->type() != JsonW::STRING)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER has no s2" << json_;
            return;
        }

        strparm = jvalue->str();

        if (strparm.length() < 5)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER contains s2 that too short" << json_;
            return;
        }

        strparms_.push_back(strparm);

        // gender
        jvalue = json_.get(u8"i1");
        if (jvalue == NULL || jvalue->type() != JsonW::INTEGER)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER has no i1" << json_;
            return;
        }

        uiparm = (uint_fast32_t)(jvalue->integer());
        if (uiparm != Player::GENDER_FEMALE && uiparm != Player::GENDER_MALE && uiparm != Player::GENDER_NEUTRAL)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER contain invalid gender:" << uiparm;
            return;
        }

        uiparms_.push_back(uiparm);

        // class
        jvalue = json_.get(u8"i2");

        if (jvalue == NULL || jvalue->type() != JsonW::INTEGER)
        {
            LOG_E(tag_) << "cons, cmd CONFIRM_CHARACTER has no i2" << json_;
            return;
        }

        uiparm = (uint_fast32_t)(jvalue->integer());
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
        jvalue = json_.get(u8"s1");
        
        if (jvalue == NULL || jvalue->type() != JsonW::STRING)
        {
            LOG_E(tag_) << "cons, cmd LOGIN has no s1" << json_;
            return;
        }

        strparm = jvalue->str();

        if (strparm.length() < 5)
        {
            LOG_E(tag_) << "cons, cmd LOGIN contains s1 that too short" << json_;
            return;
        }

        strparms_.push_back(jvalue->str());

        // password
        jvalue = json_.get(u8"s2");
        if (jvalue == NULL || jvalue->type() != JsonW::STRING)
        {
            LOG_E(tag_) << "cons, cmd LOGIN has no s2" << json_;
            return;
        }

        strparm = jvalue->str();

        if (strparm.length() < 5)
        {
            LOG_E(tag_) << "cons, cmd LOGIN contains s2 that too short" << json_;
            return;
        }

        strparms_.push_back(strparm);
        valid_ = true;
        break;       

    case LOGOUT:
        valid_ = true;
        break;

    case MOVE_NORMAL:
        // direction
        jvalue = json_.get(u8"i1");
        if (jvalue == NULL || jvalue->type() != JsonW::INTEGER)
        {
            LOG_E(tag_) << "cons, cmd MOVE_NORMAL has no i1" << json_;
            return;
        }

        uiparm = (uint_fast32_t)(jvalue->integer());
        if (uiparm != Cube::X_INC && uiparm != Cube::Y_INC && uiparm != Cube::Z_INC &&
            uiparm != Cube::X_DEC && uiparm != Cube::Y_DEC && uiparm != Cube::Z_DEC )
        {
            LOG_E(tag_) << "cons, cmd MOVE_NORMAL contain invalid direction:" << uiparm;
            return;
        }

        uiparms_.push_back(uiparm);
        valid_ = true;
        break;

    case FREEZE_WORLD:
        valid_ = true;
        break;

    default:
        // unknown cmd
        return;
    }
}

octillion::Command::~Command()
{
}