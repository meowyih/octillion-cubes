#include <string>
#include <memory>

#include "world/mob.hpp"
#include "jsonw/jsonw.hpp"

octillion::Mob::Mob()
{
    id_ = 0;
    race_ = 0;
    class_ = 0;
    level_ = 0;
}

// operator overriding
octillion::Mob& octillion::Mob::operator=(const octillion::Mob& meta)
{
    id_ = meta.id_;
    race_ = meta.race_;
    class_ = meta.class_;
    level_ = meta.level_;
    short_ = meta.short_;
    long_ = meta.long_;
    
    return *this;
}

void octillion::Mob::load( int_fast32_t areaid, std::shared_ptr<JsonW> json )
{
    std::shared_ptr<JsonW> jid, jrace, jclass, jlevel, jshort, jlong;
    
    if ( valid( json ) == false )
    {
        return;
    }
    
    jid = json->get( u8"id" );
    jrace = json->get( u8"race" );
    jclass = json->get( u8"class" );
    jlevel = json->get( u8"level" );
    jshort = json->get( u8"short" );
    jlong = json->get( u8"long" );
    
    if ( jid != nullptr && jid->type() == JsonW::INTEGER )
    {
        id_ = calc_id( areaid, (int_fast32_t)jid->integer());
    }
    
    if ( jrace != nullptr && jrace->type() == JsonW::INTEGER )
    {
        race_ = (int_fast32_t)jrace->integer();
    }
    
    if ( jclass != nullptr && jclass->type() == JsonW::INTEGER )
    {
        class_ = (int_fast32_t)jclass->integer();
    }
    
    if ( jlevel != nullptr && jlevel->type() == JsonW::INTEGER )
    {
        level_ = (int_fast32_t)jlevel->integer();
    }
    
    if ( jshort != nullptr && jshort->type() == JsonW::STRING )
    {
        short_ = jshort->str();
    }
    
    if ( jlong != nullptr && jlong->type() == JsonW::STRING )
    {
        long_ = jlong->str();
    }
}

bool octillion::Mob::valid( std::shared_ptr<JsonW> json )
{
    std::shared_ptr<JsonW> jid;
    jid = json->get( u8"id" );
    
    if ( jid == nullptr || jid->type() != JsonW::INTEGER )
    {
        return false;
    }
    
    return true;
}

int_fast32_t octillion::Mob::refid( std::shared_ptr<JsonW> json )
{
    int_fast32_t refid;
    std::shared_ptr<JsonW> jrefid;
    jrefid = json->get( u8"refid" );
    
    if ( jrefid == nullptr || jrefid->type() != JsonW::INTEGER )
    {
        return 0;
    }
    
    refid = (int_fast32_t)jrefid->integer();
    
    if ( refid > 0 )
    {
        return refid;
    }
    
    return 0;
}

int_fast32_t octillion::Mob::calc_id( int_fast32_t areaid, int_fast32_t mid )
{
    return areaid * octillion::Mob::MAX_MOB_COUNT_IN_AREA + mid;
}