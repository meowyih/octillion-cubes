
#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "jsonw/jsonw.hpp"
#include "world/event.hpp"

octillion::Event::Event()
{
}

octillion::Event::~Event()
{
}

// operator overriding
octillion::Event& octillion::Event::operator=(const octillion::Event& event)
{
    if ( this != &event )
    {
        type_ = event.type_;
        strparms_ = event.strparms_;
        valid_ = event.valid_;
        fd_ = event.fd_;
        id_ = event.id_;
    }
    
    return *this;
}

// external event from network with fd and raw data
octillion::Event::Event( int fd, std::vector<uint8_t>& data )
{
    JsonW json((const char*)data.data(), data.size());
    
    fd_ = fd;
    valid_ = false;
    
    // valid command must be a valid json text
    if ( json.valid() == false )
    {
        LOG_E(tag_) << "invalid json format";
        return;
    }
    
    // valid command must contains one and only one json object
    if (json.type() != JsonW::OBJECT)
    {
        LOG_E(tag_) << "invalid json due to no object: " << json;
        return;
    }
    
    // valid command must contains "cmd" name-value pair as integer
    std::shared_ptr<JsonW> jcmd = json.get(u8"cmd");
    if (jcmd == nullptr || jcmd->type() != JsonW::INTEGER )
    {
        LOG_E(tag_) << "cons, json's object has no cmd: " << json;
        return;
    }

    type_ = (int)(jcmd->integer());
    
    if ( type_ == TYPE_PLAYER_CREATE || type_ == TYPE_PLAYER_LOGOUT )
    {
        valid_ = true;
        return;
    }
    
    if ( type_ == TYPE_PLAYER_LOGIN )
    {
        std::shared_ptr<JsonW> juser = json.get(u8"user");
        std::shared_ptr<JsonW> jpass = json.get(u8"passwd");
        
        if ( juser == nullptr || juser->type() != JsonW::STRING || juser->str().length() == 0 )
        {
            LOG_E(tag_) << "invalid login json with no user " << json;
            return;
        }
        
        if ( jpass == nullptr || jpass->type() != JsonW::STRING || jpass->str().length() == 0 )
        {
            LOG_E(tag_) << "invalid login json with no passwd " << json;
            return;
        }
        
        strparms_.push_back( juser->str() );
        strparms_.push_back( jpass->str() );
        
        valid_ = true;
        
        return;
    }
    
    if ( type_ == TYPE_SERVER_VERIFY_TOKEN )
    {
        std::shared_ptr<JsonW> juser = json.get(u8"user");
        std::shared_ptr<JsonW> jtoken = json.get(u8"token");
        std::shared_ptr<JsonW> jip = json.get(u8"ip");
        
        if ( juser == nullptr || juser->type() != JsonW::STRING || juser->str().length() == 0 )
        {
            LOG_E(tag_) << "invalid login json with no user " << json;
            return;
        }
        
        if ( jtoken == nullptr || jtoken->type() != JsonW::STRING || jtoken->str().length() == 0 )
        {
            LOG_E(tag_) << "invalid login json with no token " << json;
            return;
        }
        
        if ( jip == nullptr || jip->type() != JsonW::STRING || jip->str().length() == 0 )
        {
            LOG_E(tag_) << "invalid login json with no ip " << json;
            return;
        }
        
        strparms_.push_back( juser->str() );
        strparms_.push_back( jtoken->str() );
        strparms_.push_back( jip->str() );
        
        valid_ = true;
        
        return;
    }
    
    if ( type_ == TYPE_PLAYER_VERIFY_TOKEN )
    {
        std::shared_ptr<JsonW> juser = json.get(u8"user");
        std::shared_ptr<JsonW> jtoken = json.get(u8"token");
        
        if ( juser == nullptr || juser->type() != JsonW::STRING || juser->str().length() == 0 )
        {
            LOG_E(tag_) << "invalid login json with no user " << json;
            return;
        }
        
        if ( jtoken == nullptr || jtoken->type() != JsonW::STRING || jtoken->str().length() == 0 )
        {
            LOG_E(tag_) << "invalid login json with no token " << json;
            return;
        }
        
        strparms_.push_back( juser->str() );
        strparms_.push_back( jtoken->str() );
        
        valid_ = true;
        
        return;
    }
    
    // other types
    valid_ = true;
}
