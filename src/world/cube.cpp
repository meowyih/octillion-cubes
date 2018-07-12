
#include <sstream>
#include <cstring> // memset
#include <system_error>
#include <map>

#include "world/cube.hpp"
#include "world/event.hpp"

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

octillion::CubePosition::CubePosition(const CubePosition& rhs, int direction)
{
    x_axis_ = rhs.x_axis_;
    y_axis_ = rhs.y_axis_;
    z_axis_ = rhs.z_axis_;

    switch (direction)
    {
    case Cube::X_INC:
        x_axis_++;
        break;
    case Cube::Y_INC:
        y_axis_++;
        break;
    case Cube::Z_INC:
        z_axis_++;
        break;
    case Cube::X_DEC:
        x_axis_--;
        break;
    case Cube::Y_DEC:
        y_axis_--;
        break;
    case Cube::Z_DEC:
        z_axis_--;
        break;    
    }
}

octillion::CubePosition::CubePosition(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
{
    x_axis_ = x;
    y_axis_ = y;
    z_axis_ = z;
}

void octillion::CubePosition::set(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
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

octillion::JsonW* octillion::CubePosition::json()
{
    JsonW* jobject = new JsonW();
    jobject->add("x", (int)x_axis_);
    jobject->add("y", (int)y_axis_);
    jobject->add("z", (int)z_axis_);
    return jobject;
}

octillion::Cube::Cube(const CubePosition& loc)
{
    loc_ = loc;
    areaid_ = 0;
    std::memset( exits_, 0, sizeof exits_);
}

octillion::Cube::Cube(const CubePosition& loc, const std::string& title, int areaid)
{
    loc_ = loc;
    title_ = title;   
    areaid_ = areaid;
    std::memset( exits_, 0, sizeof exits_);
}

octillion::Cube::Cube( const Cube& rhs )
{
    loc_ = rhs.loc_;
    title_ = rhs.title_;   
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

// parameter type
// 1 - Event::TYPE_JSON_SIMPLE, for login/logout/arrive/leave event usage
// 2 - Event::TYPE_JSON_DETAIL, for detail event usage
octillion::JsonW* octillion::Cube::json(int type)
{
    JsonW* jobject = new JsonW();
    switch (type)
    {
    case Event::TYPE_JSON_DETAIL:
    case Event::TYPE_JSON_SIMPLE:
        jobject->add("loc", loc_.json() );
        jobject->add("area", areaid_);
        break;
    }

    return jobject;
}

octillion::Area::Area( JsonW* json )
{
    std::map<std::string, CubePosition> markmap;
    JsonW* jvalue;
    
    // invalid json
    if ( json == NULL || json->valid() == false )
    {
        return;
    }
    
    // valid area json is an object
    if ( json->type() != JsonW::OBJECT )
    {
        return;
    }

    // area json must have id  
    jvalue = json->get( u8"id" );
    if (jvalue == NULL || jvalue->type() != JsonW::INTEGER )
    {
        return;     
    }
    else
    {
        id_ = (int)(jvalue->integer());
    }
    
    // area json must have title
    jvalue = json->get( u8"title" );
    if (jvalue == NULL || jvalue->type() != JsonW::STRING )
    {
        return;     
    }
    else
    {
        title_ = jvalue->str();
    }
    
    // area json must have offset array
    jvalue = json->get( u8"offset" );
    if (jvalue == NULL || jvalue->type() != JsonW::ARRAY )
    {
        return;     
    }
    else
    {
        if (jvalue->size() != 3 )
        {
            return;
        }
        
        offset_x_ = (int)(jvalue->get(0)->integer());
        offset_y_ = (int)(jvalue->get(1)->integer());
        offset_z_ = (int)(jvalue->get(2)->integer());
    }
    
    // area json must have cubes
    jvalue = json->get(u8"cubes");
    if (jvalue == NULL || jvalue->type() != JsonW::ARRAY )
    {
        return;
    }
    
    for ( size_t i = 0; i < jvalue->size(); i ++ )
    {
        bool ret;
        
        // "cubes" array must contains only object
        JsonW* jcube = jvalue->get(i);

        if (jcube == NULL ) // not possible
        {
            return;
        }
        
        // get loc and store in pos
        CubePosition pos;
        ret = readloc(jcube->get( u8"loc" ), pos, offset_x_, offset_y_, offset_z_ );
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
        JsonW* jtitle = jcube->get( u8"title" );
        if ( jtitle == NULL || jtitle->type() != JsonW::STRING || jtitle->str().length() == 0 )
        {
            return;
        }

        // get mark if exist (optional)        
        JsonW* jmark = jcube->get( u8"mark" );
        if (jcube != NULL && jmark->type() == JsonW::STRING )
        {
            std::string markstr = jmark->str();

            if (markstr.length() == 0)
            {
                return;
            }

            // duplicate mark
            if ( markmap.find(markstr) != markmap.end() )
            {
                LOG_E(tag_) << "err: duplicate cube mark " << markstr;
                return;
            }
            else
            {
                markmap[markstr] = pos;
            }
        }
                
        // create cube and store in cubes_
        Cube* cube = new Cube( pos, jtitle->str(), id_ );
        cubes_[pos] = cube;
    }
    
    // links is optional in area, although it does not make sense to create area without it
    jvalue = json->get(u8"links");
    if (jvalue != NULL && jvalue->type() == JsonW::ARRAY )
    {
        bool ret;
        for ( size_t i = 0; i < jvalue->size(); i ++ )
        {        
            JsonW* jlink = jvalue->get(i);
            JsonW* jlinkvalue;

            if (jlink == NULL || jlink->type() != JsonW::OBJECT )
            {
                return;
            }

            int type = (int)(jlink->get( u8"type" )->integer());
            if ( type == 0 )
            {
                return;
            }
            
            CubePosition from, to;

            // get 'from'
            jlinkvalue = jlink->get( u8"from" );
            if (jlinkvalue == NULL )
            {
                return;
            }
            else if (jlinkvalue->type() == JsonW::STRING )
            {
                std::string str = jlinkvalue->str();
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
            else if (jlinkvalue->type() == JsonW::ARRAY )
            {
                ret = readloc(jlinkvalue, from, offset_x_, offset_y_, offset_z_ );
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
            jlinkvalue = jlink->get( u8"to" );            
            if (jlinkvalue == NULL )
            {
                return;
            }
            else if (jlinkvalue->type() == JsonW::STRING )
            {
                std::string str = jlinkvalue->str();
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
            else if ( jlinkvalue->type() == JsonW::ARRAY )
            {
                ret = readloc(jlinkvalue, to, offset_x_, offset_y_, offset_z_ );
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
            Cube* cubefrom = cubes_[from];
            Cube* cubeto = cubes_[to];
            
            addlink(type, cubefrom, cubeto);
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

bool octillion::Area::readloc( JsonW* jvalue, CubePosition& pos, uint_fast32_t offset_x, uint_fast32_t offset_y, uint_fast32_t offset_z )
{
    uint_fast32_t x, y, z;
    if ( jvalue == NULL || jvalue->valid() == false || jvalue->type() != JsonW::ARRAY )
    {
        return false;
    }
    
    if (jvalue->size() != 3 )
    {
        return false;
    }
    
    if (jvalue->get(0)->type() != JsonW::INTEGER ||
        jvalue->get(1)->type() != JsonW::INTEGER ||
        jvalue->get(2)->type() != JsonW::INTEGER)
    {
        return false;
    }
    
    x = (int)(jvalue->get(0)->integer() + offset_x);
    y = (int)(jvalue->get(1)->integer() + offset_y);
    z = (int)(jvalue->get(2)->integer() + offset_z);
    
    pos.set( x, y, z );
    return true;
}

bool octillion::Area::addlink(int linktype, Cube* from, Cube* to)
{
    // type-2 is two-way link
    if (linktype == 2)
    {
        if (from->addlink(to) == false)
        {
            return false;
        }

        if (to->addlink(from) == false)
        {
            return false;
        }

        return true;
    }

    return false;
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

// read the mark string and cube position in "cubes" in json
bool octillion::Area::getmark(
    const JsonW* json,
    std::map<std::string, Cube*>& marks,
    const std::map<CubePosition, Cube*>& cubes )
{
    int offset_x, offset_y, offset_z;
    size_t size;
    JsonW* jcubes = json->get(u8"cubes");
    if (jcubes == NULL || jcubes->type() != JsonW::ARRAY)
    {
        return false;
    }

    // area json must have offset array
    JsonW* jvalue = json->get(u8"offset");
    if (jvalue == NULL || jvalue->type() != JsonW::ARRAY)
    {
        return false;
    }
    else
    {
        if (jvalue->size() != 3)
        {
            return false;
        }

        offset_x = (int)(jvalue->get(0)->integer());
        offset_y = (int)(jvalue->get(1)->integer());
        offset_z = (int)(jvalue->get(2)->integer());
    }

    size = jcubes->size();
    for (size_t idx = 0; idx < size; idx++)
    {
        bool ret;
        CubePosition pos;
        JsonW* jcube = jcubes->get(idx);
        JsonW* jmark = jcube->get("mark");

        if (jmark == NULL || jmark->type() != JsonW::STRING)
        {
            continue;
        }

        ret = readloc(jcube->get(u8"loc"), pos, offset_x, offset_y, offset_z);
        if (ret == false)
        {
            continue;
        }

        auto itcube = cubes.find(pos);
        if (itcube == cubes.end())
        {
            continue;
        }
        else
        {
            marks[jmark->str()] = itcube->second;
        }
    }

    return true;
}