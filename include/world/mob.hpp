#ifndef OCTILLION_MOB_HEADER
#define OCTILLION_MOB_HEADER

#include <string>
#include <memory>

#include "jsonw/jsonw.hpp"

namespace octillion
{
    class Mob;
}

class octillion::Mob
{
public:
    const static int_fast32_t MAX_MOB_COUNT_IN_AREA = 1000;  

private:
	const std::string tag_ = "Mob";

public:
    Mob();
    
    // operator overriding
    Mob& operator=(const Mob& meta);
    
    // load partial or full data from json
    void load( int_fast32_t areaid, std::shared_ptr<JsonW> json );
    
public:
    static bool valid( std::shared_ptr<JsonW> json );
    static int_fast32_t refid( std::shared_ptr<JsonW> json );
    static int_fast32_t calc_id( int_fast32_t areaid, int_fast32_t mid );
    
public:
    // area_id * MAX_MOB_COUNT_IN_AREA + mobid
    int_fast32_t id_;
    int_fast32_t race_;
    int_fast32_t class_;
    int_fast32_t level_;
    std::string  short_;
    std::string  long_;
};

#endif