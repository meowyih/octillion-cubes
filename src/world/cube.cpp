#include <sstream>
#include <cstring> // memset
#include <system_error>
#include <map>
#include <set>
#include <memory>
#include <cstdlib> // rand()

#include "world/cube.hpp"
#include "world/event.hpp"
#include "world/script.hpp"
#include "world/interactive.hpp"

#include "jsonw/jsonw.hpp"
#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

octillion::Cube::Cube(const CubePosition& loc)
{
    loc_ = loc;
    areaid_ = 0;
    std::memset( exits_, 0, sizeof exits_);
    std::memset(adjacent_cubes_, 0, sizeof adjacent_cubes_);
}

octillion::Cube::Cube(const CubePosition& loc, std::shared_ptr<octillion::StringData> title_ptr, int areaid, uint_fast32_t attr)
{
    loc_ = loc;
    areaid_ = areaid;
	attr_ = attr;
    title_ptr_ = title_ptr;
    std::memset( exits_, 0, sizeof exits_);
    std::memset(adjacent_cubes_, 0, sizeof adjacent_cubes_);
}

octillion::Cube::Cube( const Cube& rhs )
{
    loc_ = rhs.loc_;
    areaid_ = rhs.areaid_;
	attr_ = rhs.attr_; 
    title_ptr_ = rhs.title_ptr_;
    std::memcpy( exits_, rhs.exits_, sizeof exits_);
    std::memset(adjacent_cubes_, 0, sizeof adjacent_cubes_);
}

octillion::Cube::~Cube()
{
}

bool octillion::Cube::addlink(std::shared_ptr<Cube> dest, uint_fast32_t attr)
{
	CubePosition to = dest->loc();
	if (to.x() == loc_.x() + 1 && to.y() == loc_.y() && to.z() == loc_.z())
	{
		exits_[X_INC] = attr;
	}
	else if (to.x() == loc_.x() - 1 && to.y() == loc_.y() && to.z() == loc_.z())
	{
		exits_[X_DEC] = attr;
	}
	else if (to.x() == loc_.x() && to.y() == loc_.y() + 1 && to.z() == loc_.z())
	{
		exits_[Y_INC] = attr;
	}
	else if (to.x() == loc_.x() && to.y() == loc_.y() - 1 && to.z() == loc_.z())
	{
		exits_[Y_DEC] = attr;
	}
	else if (to.x() == loc_.x() && to.y() == loc_.y() && to.z() == loc_.z() + 1)
	{
		exits_[Z_INC] = attr;
	}
	else if (to.x() == loc_.x() && to.y() == loc_.y() && to.z() == loc_.z() - 1)
	{
		exits_[Z_DEC] = attr;
	}
	else
	{
		return false;
	}

	return true;
}

bool octillion::Cube::addlink(std::shared_ptr<Cube> dest)
{
	return addlink(dest, EXIT_NORMAL);
}

std::string octillion::Cube::title()
{
    if (title_ptr_ == nullptr)
        return std::string();

    return title_ptr_->str_.at(0);
}

std::wstring octillion::Cube::wtitle()
{
    if (title_ptr_ == nullptr)
        return std::wstring();

    return title_ptr_->wstr_.at(0);
}

// parameter type
// 1 - Event::TYPE_JSON_SIMPLE, for login/logout/arrive/leave event usage
// 2 - Event::TYPE_JSON_DETAIL, for detail event usage
std::shared_ptr<JsonW> octillion::Cube::json(int type)
{
	std::shared_ptr<JsonW> jobject = std::make_shared<JsonW>();
    jobject->add("loc", loc_.json() );
    jobject->add("area", areaid_);

    return jobject;
}

octillion::Area::Area( std::shared_ptr<JsonW> json )
{
    std::map<std::string, CubePosition> markmap;
    std::shared_ptr<JsonW> jvalue;
    std::shared_ptr<JsonW> jstable;
    std::shared_ptr<JsonW> jscripts;
    
    // invalid json
    if ( json == NULL || json->valid() == false )
    {
        LOG_D(tag_) << "Area init failed, json is not valid";
        return;
    }
    
    // valid area json is an object
    if ( json->type() != JsonW::OBJECT )
    {
        LOG_D(tag_) << "Area init failed, top json is a object";
        return;
    }

    // area json must have id  
    jvalue = json->get( u8"id" );
    if (jvalue == NULL || jvalue->type() != JsonW::INTEGER )
    {
        LOG_D(tag_) << "Area init failed, json has no id field";
        return;     
    }
    else
    {
        id_ = (int)(jvalue->integer());
    }

    // check string table
    jstable = json->get(u8"strings");
    if (jstable != nullptr)
    {
        bool success = string_table_.add(jstable);
        if (!success)
        {
            LOG_W(tag_) << "Area " << title_ << " has no string table";
        }
    }
    
    // area json must have title
    jvalue = json->get( u8"title" );
    if (jvalue != nullptr && jvalue->type() == JsonW::INTEGER)
    {
        int strid = (int)(jvalue->integer());

        if (string_table_.find(strid)->str_.size() == 0 || string_table_.find(strid)->wstr_.size() == 0)
        {
            LOG_D(tag_) << "Area init failed, area title does not have good string table id:" << strid;
            return;
        }

        title_ = string_table_.find(strid)->str_[0];
        wtitle_ = string_table_.find(strid)->wstr_[0];
    }
    else
    {
        LOG_D(tag_) << "Area init failed, json has no valid title field";
        return;
    }

    // area json must have offset array
    jvalue = json->get( u8"offset" );
    if (jvalue == NULL || jvalue->type() != JsonW::ARRAY )
    {
        LOG_D(tag_) << "Area init failed, json has no offset field";
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

    // read the script
    jscripts = json->get(u8"scripts");
    if (jscripts != nullptr && jscripts->type() == JsonW::ARRAY && jscripts->size() > 0)
    {
        for (size_t i = 0; i < jscripts->size(); i++)
        {
            octillion::Script script;
            bool success = script.init(jscripts->get(i), markmap, offset_x_, offset_y_, offset_z_);
            if (!success)
            {
                LOG_W(tag_) << "Area " << title_ << " has corrupted script data";
            }
            else
            {
                scripts_.push_back(script);
            }
        }
    }

    // read interactives
    jvalue = json->get(u8"interactives");
    if (jvalue != nullptr && jvalue->type() == JsonW::ARRAY)
    {
        for (size_t i = 0; i < jvalue->size(); i++)
        {
            std::shared_ptr<octillion::Interactive> interactive
                = std::make_shared<octillion::Interactive>();

            bool ret = interactive->init(id(), jvalue->get(i), markmap, offset_x_, offset_y_, offset_z_, string_table_);
            if (ret == false)
            {
                LOG_E(tag_) << "Area " << title_ << " detect bad interactive " << jvalue->text();
                return;
            }
            interactives_.push_back(interactive);
        }

        LOG_D(tag_) << "Read " << jvalue->size() << " interactives in " << title_ << " id:" << id_;
    }
    
    // area json must have cubes
    jvalue = json->get(u8"cubes");
    if (jvalue == nullptr || jvalue->type() != JsonW::ARRAY )
    {
        LOG_D(tag_) << "Area init failed, json has no cube field";
        return;
    }
    
    for ( size_t i = 0; i < jvalue->size(); i ++ )
    {
        bool ret;
        std::shared_ptr<octillion::StringData> title_ptr;
        
        // "cubes" array must contains only object
        std::shared_ptr<JsonW> jcube = jvalue->get(i);

        if (jcube == NULL ) // not possible
        {
            LOG_D(tag_) << "Area init failed, json has a cube that is not an object";
            return;
        }
        
        // get loc and store in pos
        CubePosition pos;
        ret = readloc(jcube->get( u8"loc" ), pos, offset_x_, offset_y_, offset_z_ );
        if ( ret == false )
        {
            LOG_D(tag_) << "Area init failed, json has a cube with bad loc";
            return;
        }
        
        // check if duplicate cube
        if ( cubes_.find( pos ) != cubes_.end() )
        {
            LOG_E(tag_) << "err: duplicate cube, pos x:" << pos.x() << " y:" << pos.y() << " z:" << pos.z();
            return;
        }
        
        // get title
        std::shared_ptr<JsonW> jtitle = jcube->get( u8"title" );
        if (jtitle != nullptr && jtitle->type() == JsonW::INTEGER )        
        {
            int strid = (int)(jtitle->integer());
            title_ptr = string_table_.find(strid);
        }
        
        // get mark if exist (optional)        
        std::shared_ptr<JsonW> jmark = jcube->get( u8"mark" );
        if (jmark != NULL && jmark->type() == JsonW::STRING )
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

		// get attr if exist (optional)
		std::shared_ptr<JsonW> jattrs = jcube->get(u8"attr");
		uint_fast32_t attr;
		if (Cube::json2attr(jattrs, attr) != OcError::E_SUCCESS)
		{
			attr = 0xFFFFFFFF;
		}
                
        // create cube and store in cubes_
        std::shared_ptr<Cube> cube = std::make_shared<Cube>( pos, title_ptr, id_, attr);
        cubes_[pos] = cube;

        // read exits
        std::shared_ptr<JsonW> jexits = jcube->get(u8"exits");
        if (jexits != nullptr && jexits->type() == JsonW::STRING)
        {
            std::string exits = jexits->str();
            if (exits.find('n') != std::string::npos)
                cube->exits_[octillion::Cube::Y_INC] = octillion::Cube::EXIT_NORMAL;
            if (exits.find('e') != std::string::npos)
                cube->exits_[octillion::Cube::X_INC] = octillion::Cube::EXIT_NORMAL;
            if (exits.find('s') != std::string::npos)
                cube->exits_[octillion::Cube::Y_DEC] = octillion::Cube::EXIT_NORMAL;
            if (exits.find('w') != std::string::npos)
                cube->exits_[octillion::Cube::X_DEC] = octillion::Cube::EXIT_NORMAL;
            if (exits.find('u') != std::string::npos)
                cube->exits_[octillion::Cube::Z_INC] = octillion::Cube::EXIT_NORMAL;
            if (exits.find('d') != std::string::npos)
                cube->exits_[octillion::Cube::Z_DEC] = octillion::Cube::EXIT_NORMAL;
        }

        // read exits' desc (options)
        std::shared_ptr<JsonW> jdesc = jcube->get(u8"nstr");
        if (jdesc != nullptr && jdesc->type() == JsonW::INTEGER)
        {
            int strid = (int)(jdesc->integer());
            cube->desc_exits_[octillion::Cube::Y_INC] = string_table_.find(strid);
        }

        jdesc = jcube->get(u8"estr");
        if (jdesc != nullptr && jdesc->type() == JsonW::INTEGER)
        {
            int strid = (int)(jdesc->integer());
            cube->desc_exits_[octillion::Cube::X_INC] = string_table_.find(strid);
        }

        jdesc = jcube->get(u8"wstr");
        if (jdesc != nullptr && jdesc->type() == JsonW::INTEGER)
        {
            int strid = (int)(jdesc->integer());
            cube->desc_exits_[octillion::Cube::X_DEC] = string_table_.find(strid);
        }

        jdesc = jcube->get(u8"sstr");
        if (jdesc != nullptr && jdesc->type() == JsonW::INTEGER)
        {
            int strid = (int)(jdesc->integer());
            cube->desc_exits_[octillion::Cube::Y_DEC] = string_table_.find(strid);
        }

        jdesc = jcube->get(u8"ustr");
        if (jdesc != nullptr && jdesc->type() == JsonW::INTEGER)
        {
            int strid = (int)(jdesc->integer());
            cube->desc_exits_[octillion::Cube::Z_INC] = string_table_.find(strid);
        }

        jdesc = jcube->get(u8"dstr");
        if (jdesc != nullptr && jdesc->type() == JsonW::INTEGER)
        {
            int strid = (int)(jdesc->integer());
            cube->desc_exits_[octillion::Cube::Z_DEC] = string_table_.find(strid);
        }
    }
    
    // links is optional in area, although it does not make sense to create area without it
    jvalue = json->get(u8"links");
    if (jvalue != NULL && jvalue->type() == JsonW::ARRAY )
    {
        bool ret;
		for (size_t i = 0; i < jvalue->size(); i++)
		{
			std::shared_ptr<JsonW> jlink = jvalue->get(i);
			std::shared_ptr<JsonW> jlinkvalue;

			if (jlink == NULL || jlink->type() != JsonW::OBJECT)
			{
                LOG_D(tag_) << "Area init failed, json has a link which is not an object";
				return;
			}

			bool twoway = true;
			std::shared_ptr<JsonW> jtype = jlink->get(u8"type");
			if (jtype != NULL && jtype->type() == JsonW::STRING)
			{
				if (jtype->str() == u8"1way")
				{
					twoway = false;
				}
			}

			CubePosition from, to;

			// get 'from'
			jlinkvalue = jlink->get(u8"from");
			if (jlinkvalue == NULL)
			{
                LOG_E(tag_) << "Area init failed, json has a link with bad from field";
				return;
			}
			else if (jlinkvalue->type() == JsonW::STRING)
			{
				std::string str = jlinkvalue->str();
				auto it = markmap.find(str);
				if (it == markmap.end())
				{
                    LOG_E(tag_) << "Area init failed, json has a link with bad type field";
					return;
				}
				else
				{
					from = it->second;
				}
			}
			else if (jlinkvalue->type() == JsonW::ARRAY)
			{
				ret = readloc(jlinkvalue, from, offset_x_, offset_y_, offset_z_);
				if (ret == false)
				{
                    LOG_E(tag_) << "Area init failed, json has a link with bad array field";
					return;
				}
			}
			else
			{
                LOG_E(tag_) << "Area init failed, json has a link with unknwon represenation";
				return;
			}

			// get 'to'
			jlinkvalue = jlink->get(u8"to");
			if (jlinkvalue == NULL)
			{
                LOG_E(tag_) << "Area init failed, json has a link with bad to field";
				return;
			}
			else if (jlinkvalue->type() == JsonW::STRING)
			{
				std::string str = jlinkvalue->str();
				auto it = markmap.find(str);
				if (it == markmap.end())
				{
                    LOG_E(tag_) << "Area init failed, json has a link-to field with undefined mark";
					return;
				}
				else
				{
					to = it->second;
				}
			}
			else if (jlinkvalue->type() == JsonW::ARRAY)
			{
				ret = readloc(jlinkvalue, to, offset_x_, offset_y_, offset_z_);
				if (ret == false)
				{
                    LOG_E(tag_) << "Area init failed, json has a link-to field with loc array";
					return;
				}
			}
			else
			{
                LOG_E(tag_) << "Area init failed, json has a link-to field with unknwon representation";
				return;
			}

			// check if from and to both exists
			if (cubes_.find(from) == cubes_.end())
			{
                LOG_E(tag_) << "Area init failed, json has a from field has no cube";
				return;
			}

			if (cubes_.find(to) == cubes_.end())
			{
                LOG_E(tag_) << "Area init failed, json has a to field has no cube";
				return;
			}

			// get attr if exist (optional)
			std::shared_ptr<JsonW> jattrs = jlink->get(u8"attr");
			bool hasattrs = false;
			uint_fast32_t attr;
			if (Cube::json2attr(jattrs, attr) == OcError::E_SUCCESS)
			{
				hasattrs = true;
			}

			// add link for each cube
			std::shared_ptr<Cube> cubefrom = cubes_[from];
			std::shared_ptr<Cube> cubeto = cubes_[to];

			if (hasattrs)
			{
				addlink(twoway, cubefrom, cubeto, attr);
			}
			else
			{
				addlink(twoway, cubefrom, cubeto);	
			}
        }
    }
    
    valid_ = true;
    return;
}

std::error_code octillion::Cube::json2attr(std::shared_ptr<JsonW> jattrs, uint_fast32_t& attr)
{
	if (jattrs == NULL || jattrs->type() != JsonW::ARRAY )
	{
		return OcError::E_FATAL;
	}

	for (size_t idx = 0; idx < jattrs->size(); idx++)
	{
		std::shared_ptr<JsonW> jattr = jattrs->get(idx);
		std::string attrstr = jattr->str();

		if (attrstr == "nomob")
		{
			attr = attr ^ Cube::MOB_CUBE;
		}
		else if (attrstr == "nonpc")
		{
			attr = attr ^ Cube::NPC_CUBE;
		}
	}
	return OcError::E_SUCCESS;
}

std::shared_ptr<octillion::Cube> octillion::Area::cube(CubePosition loc)
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

bool octillion::Area::readloc( const std::shared_ptr<JsonW> jvalue, CubePosition& pos, uint_fast32_t offset_x, uint_fast32_t offset_y, uint_fast32_t offset_z )
{
    uint_fast32_t x, y, z;
    if ( jvalue == nullptr || jvalue->valid() == false || jvalue->type() != JsonW::ARRAY )
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

bool octillion::Area::addlink(bool is_2way, std::shared_ptr<Cube> from, std::shared_ptr<Cube> to, uint_fast32_t attr )
{
	// 2-way link
	if (is_2way)
	{
		if (from->addlink(to, attr) == false)
		{
			return false;
		}

		if (to->addlink(from, attr) == false)
		{
			return false;
		}

		return true;
	}
	else
	{
		return from->addlink(to, attr);
	}
}

bool octillion::Area::addlink(bool is_2way, std::shared_ptr<Cube> from, std::shared_ptr<Cube> to)
{
    // 2-way link
    if (is_2way)
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
	else
	{
		return from->addlink(to);
	}    
}

octillion::Area::~Area()
{
}

// read the mark string and cube position in "cubes" value
bool octillion::Area::getmark(
        const std::shared_ptr<JsonW> json, 
        std::map<std::string, std::shared_ptr<Cube>>& marks,
        const std::map<CubePosition, std::shared_ptr<Cube>>& cubes)
{ 
    // static func cannot access area::tag_, create a fake one
    std::string tag_ = "Area";
    
    int offset_x, offset_y, offset_z;
    size_t size;
    
    std::string areaid = std::to_string(json->get(u8"id")->integer());
    
    std::shared_ptr<JsonW> jcubes = json->get(u8"cubes");    
    if (jcubes == nullptr || jcubes->type() != JsonW::ARRAY)
    {
        return false;
    }

    // area json must have offset array
    std::shared_ptr<JsonW> jvalue = json->get(u8"offset");
    if (jvalue == nullptr || jvalue->type() != JsonW::ARRAY)
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
        std::shared_ptr<JsonW> jcube = jcubes->get(idx);
        std::shared_ptr<JsonW> jmark = jcube->get("mark");

        if (jmark == nullptr || jmark->type() != JsonW::STRING)
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
            std::string mark = jmark->str();
            
            if ( mark.find( '@' ) != std::string::npos )
            {
                LOG_E("Area") << "Invalid area data, id " << areaid << ", mark must not contain '@'";
                return false;
            }
            
            // format 'mark' as areaid@mark
            mark = areaid + std::string( "@" ) + jmark->str();
            
            std::shared_ptr<Cube> cube = itcube->second;
            
            if ( cube == nullptr )
            {
                octillion::CubePosition pos = itcube->first;
                LOG_E("Area") << "Fatal error, cube is nullptr for " << pos.json()->text();
                return false;
            }
            
            if ( marks.find( mark ) != marks.end())
            {
                LOG_E("Area") << "Fatal error, duplicate mark " << mark;
                return false;
            }
            
            marks.insert(std::pair<std::string, std::shared_ptr<Cube>>(mark, cube));
        }
    }

    return true;
}