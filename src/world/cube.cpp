
#include <sstream>
#include <cstring> // memset
#include <system_error>
#include <map>

#include "world/cube.hpp"
#include "jsonw/jsonw.hpp"
#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

octillion::CubePosition::CubePosition()
{
    x_axis_ = 0;
    y_axis_ = 0;
    z_axis_ = 0;
}

octillion::CubePosition::CubePosition(const CubePosition& rhs)
{
    x_axis_ = rhs.x_axis_;
    y_axis_ = rhs.y_axis_;
    z_axis_ = rhs.z_axis_;
}

octillion::CubePosition::CubePosition(uint32_t x, uint32_t y, uint32_t z)
{
    x_axis_ = x;
    y_axis_ = y;
    z_axis_ = z;
}

void octillion::CubePosition::set(uint32_t x, uint32_t y, uint32_t z)
{
    x_axis_ = x;
    y_axis_ = y;
    z_axis_ = z;
}

std::string octillion::CubePosition::str()
{
    std::ostringstream oss;
    oss << "(" << x_axis_ << "," << y_axis_ << "," << z_axis_ << ")";
    return oss.str();
}

octillion::Cube::Cube(CubePosition loc)
{
    loc_ = loc;
    areaid_ = 0;
    std::memset( exits_, 0, sizeof exits_);
}

octillion::Cube::Cube(CubePosition loc, std::wstring wtitle, int areaid)
{
    loc_ = loc;
    wtitle_ = wtitle;   
    areaid_ = areaid;
    std::memset( exits_, 0, sizeof exits_);
}

octillion::Cube::Cube( const Cube& rhs )
{
    loc_ = rhs.loc_;
    wtitle_ = rhs.wtitle_;   
    areaid_ = rhs.areaid_;
    std::memcpy( exits_, rhs.exits_, sizeof exits_);
}

octillion::Cube::~Cube()
{
}

bool octillion::Cube::addlink(Cube* dest)
{
    CubePosition to = dest->loc();
    if (to.x() == loc_.x() + 1 && to.y() == loc_.y() && to.z() == loc_.z())
    {
        exits_[1] = 1;
    }
    else if (to.x() == loc_.x() - 1 && to.y() == loc_.y() && to.z() == loc_.z())
    {
        exits_[3] = 1;
    }
    else if (to.x() == loc_.x() && to.y() == loc_.y() + 1 && to.z() == loc_.z())
    {
        exits_[2] = 1;
    }
    else if (to.x() == loc_.x() && to.y() == loc_.y() - 1 && to.z() == loc_.z())
    {
        exits_[0] = 1;
    }
    else if (to.x() == loc_.x() && to.y() == loc_.y() && to.z() == loc_.z() + 1)
    {
        exits_[4] = 1;
    }
    else if (to.x() == loc_.x() && to.y() == loc_.y() && to.z() == loc_.z() - 1)
    {
        exits_[5] = 1;
    }
    else
    {
        return false;
    }

    return true;
}


octillion::Area::Area( JsonTextW* json )
{
    std::map<std::string, CubePosition> markmap;
    JsonObjectW *object = json->value()->object();
    JsonValueW* value;
    JsonArrayW *array;
     
    // invalid json
    if ( json == NULL || json->valid() == false )
    {
        return;
    }
    
    // valid area json is an object
    if ( json->value()->type() != JsonValueW::Type::JsonObject )
    {
        return;
    }

    // area json must have id  
    value = object->find( u8"id" );
    if ( value == NULL || value->type() != JsonValueW::Type::NumberInt )
    {
        return;     
    }
    else
    {
        id_ = value->integer();
    }
    
    // area json must have title
    value = object->find( u8"title" );
    if ( value == NULL || value->type() != JsonValueW::Type::String )
    {
        return;     
    }
    else
    {
        wtitle_ = value->wstring();
    }
    
    // area json must have offset array
    value = object->find( u8"offset" );
    if ( value == NULL || value->type() != JsonValueW::Type::JsonArray )
    {
        return;     
    }
    else
    {
        array = value->array();
        
        if ( array->size() != 3 )
        {
            return;
        }
        
        offset_x_ = array->at(0)->integer();
        offset_y_ = array->at(1)->integer();
        offset_z_ = array->at(2)->integer();
    }
    
    // area json must have cubes
    array = object->find( u8"cubes" )->array();
    if ( array == NULL )
    {
        return;
    }
    
    for ( size_t i = 0; i < array->size(); i ++ )
    {
        bool ret;
        
        // "cubes" array must contains only object
        JsonObjectW* cubeobj = array->at(i)->object();        
        if ( cubeobj == NULL )
        {
            return;
        }
        
        // get loc and store in pos
        CubePosition pos;
        ret = readloc( cubeobj->find( u8"loc" ), pos, offset_x_, offset_y_, offset_z_ );        
        if ( ret == false )
        {
            return;
        }
        
        // check if duplicate cube
        if ( cubes_.find( pos ) != cubes_.end() )
        {
            LOG_E(tag_) << "err: duplicate cube, pos x:" << pos.x() << " y:" << pos.y() << " z:" << pos.z();
            return;
        }
        
        // get title
        JsonValueW* jtitle = cubeobj->find( u8"title" );
        if ( jtitle == NULL || jtitle->type() != JsonValueW::Type::String || jtitle->string().length() == 0 )
        {
            return;
        }

        // get mark if exist (optional)        
        JsonValueW* mark = cubeobj->find( u8"mark" );
        if ( mark != NULL && mark->type() == JsonValueW::Type::String )
        {
            std::string str = mark->string();

            if (str.length() == 0)
            {
                return;
            }

            // duplicate map
            if ( markmap.find( mark->string() ) != markmap.end() )
            {
                LOG_E( tag_ ) << "err: duplicate cube mark " << mark->string();
                return;
            }
            else
            {
                markmap[ mark->string() ] = pos;
            }
        }
                
        // create cube and store in cubes_
        Cube* cube = new Cube( pos, jtitle->wstring(), id_ );
        cubes_[pos] = cube;
    }
    
    // links is optional in area, although it does not make sense to create area without it
    array = object->find( u8"links" )->array();
    if ( array != NULL )
    {
        bool ret;
        for ( size_t i = 0; i < array->size(); i ++ )
        {        
            JsonObjectW* link = array->at(i)->object();
            JsonValueW* linkvalue;

            if (link == NULL)
            {
                return;
            }

            int type = link->find( u8"type" )->integer();            
            if ( type == 0 )
            {
                return;
            }
            
            CubePosition from, to;

            // get 'from'
            linkvalue = link->find( u8"from" );            
            if ( linkvalue == NULL )
            {
                return;
            }
            else if ( linkvalue->type() == JsonValueW::Type::String )
            {
                std::string str = linkvalue->string();
                auto it = markmap.find( str );
                if ( it == markmap.end() )
                {
                    return;
                }
                else
                {
                    from = it->second;
                }
            }
            else if ( linkvalue->type() == JsonValueW::Type::JsonArray )
            {
                ret = readloc( linkvalue, from, offset_x_, offset_y_, offset_z_ );
                if ( ret == false )
                {
                    return;
                }
            }
            else
            {
                return;
            }
            
            // get 'to'
            linkvalue = link->find( u8"to" );            
            if ( linkvalue == NULL )
            {
                return;
            }
            else if ( linkvalue->type() == JsonValueW::Type::String )
            {
                std::string str = linkvalue->string();
                auto it = markmap.find( str );
                if ( it == markmap.end() )
                {
                    return;
                }
                else
                {
                    to = it->second;
                }
            }
            else if ( linkvalue->type() == JsonValueW::Type::JsonArray )
            {
                ret = readloc( linkvalue, to, offset_x_, offset_y_, offset_z_ );
                if ( ret == false )
                {
                    return;
                }
            }
            else
            {
                return;
            }
            
            // check if from and to both exists
            if ( cubes_.find( from ) == cubes_.end() )
            {
                return;
            }
            
            if ( cubes_.find( to ) == cubes_.end() )
            {
                return;
            }
            
            // add link for each cube
            Cube* cube1 = cubes_[from];
            Cube* cube2 = cubes_[to];
            
            // type-2 is two-way link
            if ( type == 2 )
            {
                if ( cube1->addlink( cube2 ) == false )
                {
                    return;
                }
                
                if ( cube2->addlink( cube1 ) == false )
                {
                    return;
                }
            }
        }
    }
    
    valid_ = true;
    return;
}

octillion::Cube* octillion::Area::cube(CubePosition loc)
{
    auto it = cubes_.find(loc);

    if (it == cubes_.end())
    {
        return NULL;
    }
    else
    {
        return it->second;
    }
}

bool octillion::Area::readloc( JsonValueW* jvalue, CubePosition& pos, uint32_t offset_x, uint32_t offset_y, uint32_t offset_z )
{
    JsonArrayW* jarray;
    uint32_t x, y, z;
    if ( jvalue == NULL || jvalue->valid() == false || jvalue->type() != JsonValueW::Type::JsonArray )
    {
        return false;
    }
    
    jarray = jvalue->array();
    
    if ( jarray->size() != 3 )
    {
        return false;
    }
    
    if ( jarray->at(0)->type() != JsonValueW::Type::NumberInt ||
        jarray->at(1)->type() != JsonValueW::Type::NumberInt ||
        jarray->at(2)->type() != JsonValueW::Type::NumberInt)
    {
        return false;
    }
    
    x = jarray->at(0)->integer() + offset_x;
    y = jarray->at(1)->integer() + offset_y;
    z = jarray->at(2)->integer() + offset_z;
    
    if ( x < 0 || y < 0 || z < 0 )
    {
        return false;
    }
    
    pos.set( x, y, z );
    return true;
}

octillion::Area::~Area()
{
    for (auto& it : cubes_)
    {
        if (it.second != NULL)
        {
            delete it.second;
        }
    }
}