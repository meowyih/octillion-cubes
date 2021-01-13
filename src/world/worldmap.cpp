
#include <memory>
#include <iostream>
#include <fstream>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

#include "world/cube.hpp"
#include "world/worldmap.hpp"
#include "world/mob.hpp"
#include "world/stringtable.hpp"

#include "jsonw/jsonw.hpp"

octillion::WorldMap::WorldMap()
{
    initialized_ = false;
}

octillion::WorldMap::~WorldMap()
{
}

std::shared_ptr<octillion::Cube> octillion::WorldMap::readloc(
        std::shared_ptr<JsonW> json,
        std::map<std::string, std::shared_ptr<octillion::Cube>>& marks,
        std::map<octillion::CubePosition, std::shared_ptr<octillion::Cube>> cubes )
{
    std::string tag_ = "WorldMap"; // fake tag_ for static function
    int areaid;
    std::shared_ptr<JsonW> jarea = json->get(u8"area");
    std::shared_ptr<JsonW> jloc = json->get(u8"cube");    
    if (jarea == nullptr || jarea->valid() == false || jarea->type() != JsonW::INTEGER )
    {
        areaid = 0;
    }
    else
    {
        areaid = (int)jarea->integer();
    }

    if (jloc->type() == JsonW::STRING && areaid > 0 )
    {
        std::string mark = std::to_string( areaid ) + std::string( "@" ) + jloc->str();
        auto it = marks.find(mark);
        if (it == marks.end())
        {
            LOG_E(tag_) << "readloc() mark:" << mark << " is not in marks";
            return nullptr;
        }

        return it->second;
    }
    else if (jloc->type() == JsonW::ARRAY)
    {
        std::shared_ptr<JsonW> joffset = json->get(u8"offset");
        bool ret;
        octillion::CubePosition offset;
        octillion::CubePosition cubepos;

        if (joffset != nullptr && joffset->type() == JsonW::ARRAY &&
            joffset->size() == 3)
        {
            offset.set(
                (uint_fast32_t) joffset->get(0)->integer(),
                (uint_fast32_t) joffset->get(1)->integer(),
                (uint_fast32_t) joffset->get(2)->integer());
        }

        ret = octillion::Area::readloc(jloc, cubepos, offset.x(), offset.y(), offset.z());
        if (ret == false)
        {
            LOG_E(tag_) << "readloc(), Area::readloc() failed";
            return nullptr;
        }

        auto itcube = cubes.find(cubepos);
        if (itcube == cubes.end())
        {
            LOG_E(tag_) << "cubepos " << cubepos.str() << " not in cubes";
            return NULL;
        }
        return itcube->second;
    }
    else
    {
        LOG_E(tag_) << "readloc() bad json type";
        return NULL;
    }
}

bool octillion::WorldMap::load_external_data_file( std::string directory, bool utf16 )
{
    // global configuration json file name
    std::string global_config_filepath = directory + global_config_filename_;
    
    // area id and area json file name
    std::map<int, std::string> area_files;
    
    // mark vector
    std::map<std::string, std::shared_ptr<Cube>> marks;
    
    octillion::CubePosition pos;
    
    // read global data
	std::ifstream fin(global_config_filepath);
	if (!fin.good())
	{
		LOG_E(tag_) << "Failed to open " << global_config_filepath;
		return false;
	}
    
    std::shared_ptr<JsonW> jglobal = std::make_shared<JsonW>(fin);
	if ( jglobal->valid() == false )
	{
		LOG_E(tag_) << "Failed to read " << global_config_filename_;
		return false;
	}
    
    // read stamp value in config
	global_config_stamp_ = jglobal->get(u8"stamp")->str();
	if (global_config_stamp_.length() == 0)
	{
		LOG_E(tag_) << "Failed to get stamp from " << global_config_filename_;
		return false;
	}
    
    // read areas' id and json file name in config
    std::shared_ptr<JsonW> jarea_files = jglobal->get(u8"area");
	if (jarea_files->valid() == false || jarea_files->type() != JsonW::ARRAY || jarea_files->size() == 0)
	{
		LOG_E(tag_) << "Failed to get area from " << global_config_filename_;
		return false;
	}
    
	for (size_t i = 0; i < jarea_files->size(); i++)
	{
		std::shared_ptr<JsonW> jarea_file = jarea_files->get(i);
		std::shared_ptr<JsonW> jid = jarea_file->get(u8"id");
		std::shared_ptr<JsonW> jstamp = jarea_file->get(u8"stamp");
		std::shared_ptr<JsonW> jfile = jarea_file->get(u8"file");

		if (jid == nullptr || jstamp == nullptr || jfile == nullptr || 
			jid->integer() == 0 || jstamp->str().length() == 0 || jfile->str().length() == 0)
		{
			LOG_E(tag_) << "Failed to get area file data from " << global_config_filename_;
			return false;
		}

		auto it = area_files.find((int)jid->integer());
		if (it != area_files.end())
		{
			LOG_E(tag_) << "Failed to duplicate area id " << jid->integer();
			return false;
		}

		area_files[(int)jid->integer()] = directory + jfile->str();
	}
    
    for ( auto it = area_files.begin(); it != area_files.end(); it ++ )
    {
        LOG_I(tag_) << "area id:" << it->first << " filename:" << it->second;
    }
    
    for ( auto it = area_files.begin(); it != area_files.end(); it ++ )
    {
        // read area data
        std::ifstream fin((*it).second);

        if (!fin.good())
        {
			LOG_E(tag_) << "Failed to open area file:" << (*it).second;
			return false;
        }
        
        std::shared_ptr<JsonW> json = std::make_shared<JsonW>(fin);
        if (json->valid() == false)
        {
			LOG_E(tag_) << "Failed to read area json file:" << (*it).second;
			return false;
        }

        // read area cubes
        std::shared_ptr<octillion::Area> area = std::make_shared<octillion::Area>(json);

        if (area->valid())
        {
            areas_.insert(std::pair<int, std::shared_ptr<octillion::Area>>(area->id(), area));
        }
        else
        {
            LOG_E(tag_) << "failed to load area file: " << (*it).second;
            return false;
        }
        
        LOG_I(tag_) << "load area:" << area->id() << " contains cubes:" << area->cubes_.size();
        
        if ( Area::getmark(json, marks, area->cubes_) == false )
        {
            LOG_E(tag_) << "failed to load marks in area file: " << (*it).second;
            return false;
        }

        // read area mob meta
        std::shared_ptr<JsonW> jmobs = json->get( u8"mobs" );
        
        if ( jmobs != nullptr && jmobs->type() == JsonW::ARRAY && jmobs->size() > 0 )
        {
            for ( size_t idx = 0; idx < jmobs->size(); idx ++ )
            {
                octillion::Mob mob;
                
                int_fast32_t refid;
                
                if ( ! octillion::Mob::valid( jmobs->get(idx) ))
                {
                    LOG_E(tag_) << "failed to load mobs in area file: " << (*it).second;
                    return false;
                }
                
                refid = octillion::Mob::refid( jmobs->get(idx) );

                if ( refid != 0 )
                {
                    // TODO: find the mob with refid, then mob = mob(refid)
                    auto itmob = mobs_.find( octillion::Mob::calc_id( area->id(), refid ));
                    
                    if ( itmob == mobs_.end() )
                    {
                        LOG_E(tag_) << "failed to load mobs in area file: " 
                            << (*it).second << " mob's refid " << refid;
                        return false;
                    }
                    
                    mob = (*itmob).second;
                }
                
                mob.load( area->id(), jmobs->get(idx));
                
                mobs_.insert( std::pair<int_fast32_t,octillion::Mob>(mob.id_, mob));
            }
        }

        // read mob
        for ( auto mob = mobs_.begin(); mob != mobs_.end(); mob ++ )
        {
            LOG_I(tag_) << "mob id:" << (*mob).first 
                << " short:" << (*mob).second.short_ 
                << " long:" << (*mob).second.long_;
        }

        LOG_I(tag_) << "load area:" << area->id() << " contains mob:" << mobs_.size();

        fin.close();
    }
  
    // create cubes short cut to areas_
    for (const auto& it : areas_)
    {
        for (const auto& mapit : it.second->cubes_)
        {
            cubes_[mapit.first] = mapit.second;
        }
    }

    // check the adjacent for all cubes
    for (auto itcube = cubes_.begin(); itcube != cubes_.end(); itcube++)
    {
        if ((*itcube).second->find(cubes_, octillion::Cube::X_INC) != nullptr)
            (*itcube).second->adjacent_cubes_[octillion::Cube::X_INC] = octillion::Cube::EXIT_NORMAL;
        if ((*itcube).second->find(cubes_, octillion::Cube::X_DEC) != nullptr)
            (*itcube).second->adjacent_cubes_[octillion::Cube::X_DEC] = octillion::Cube::EXIT_NORMAL;
        if ((*itcube).second->find(cubes_, octillion::Cube::Y_INC) != nullptr)
            (*itcube).second->adjacent_cubes_[octillion::Cube::Y_INC] = octillion::Cube::EXIT_NORMAL;
        if ((*itcube).second->find(cubes_, octillion::Cube::Y_DEC) != nullptr)
            (*itcube).second->adjacent_cubes_[octillion::Cube::Y_DEC] = octillion::Cube::EXIT_NORMAL;
        if ((*itcube).second->find(cubes_, octillion::Cube::Z_INC) != nullptr)
            (*itcube).second->adjacent_cubes_[octillion::Cube::Z_INC] = octillion::Cube::EXIT_NORMAL;
        if ((*itcube).second->find(cubes_, octillion::Cube::Z_DEC) != nullptr)
            (*itcube).second->adjacent_cubes_[octillion::Cube::Z_DEC] = octillion::Cube::EXIT_NORMAL;
        if ((*itcube).second->find(cubes_, octillion::Cube::X_INC_Y_INC) != nullptr)
        {
            auto adjcube = (*itcube).second->find(cubes_, octillion::Cube::X_INC_Y_INC);
            if ( adjcube->exits_[octillion::Cube::X_DEC] != 0 && adjcube->exits_[octillion::Cube::Y_DEC] != 0 )
                (*itcube).second->adjacent_cubes_[octillion::Cube::X_INC_Y_INC] = octillion::Cube::EXIT_NORMAL;
        }
        if ((*itcube).second->find(cubes_, octillion::Cube::X_DEC_Y_INC) != nullptr)
        {
            auto adjcube = (*itcube).second->find(cubes_, octillion::Cube::X_DEC_Y_INC);
            if (adjcube->exits_[octillion::Cube::X_INC] != 0 && adjcube->exits_[octillion::Cube::Y_DEC] != 0)
                (*itcube).second->adjacent_cubes_[octillion::Cube::X_DEC_Y_INC] = octillion::Cube::EXIT_NORMAL;
        }
        if ((*itcube).second->find(cubes_, octillion::Cube::X_INC_Y_DEC) != nullptr)
        {
            auto adjcube = (*itcube).second->find(cubes_, octillion::Cube::X_INC_Y_DEC);
            if (adjcube->exits_[octillion::Cube::X_DEC] != 0 && adjcube->exits_[octillion::Cube::Y_INC] != 0)
                (*itcube).second->adjacent_cubes_[octillion::Cube::X_INC_Y_DEC] = octillion::Cube::EXIT_NORMAL;
        }
        if ((*itcube).second->find(cubes_, octillion::Cube::X_DEC_Y_DEC) != nullptr)
        {
            auto adjcube = (*itcube).second->find(cubes_, octillion::Cube::X_DEC_Y_DEC);
            if (adjcube->exits_[octillion::Cube::X_INC] != 0 && adjcube->exits_[octillion::Cube::Y_INC] != 0)
                (*itcube).second->adjacent_cubes_[octillion::Cube::X_DEC_Y_DEC] = octillion::Cube::EXIT_NORMAL;
        }
    }

    // read reborn loc
    std::shared_ptr<JsonW> jreborn = jglobal->get(u8"reborn");
    
    if (jreborn == nullptr || jreborn->type() != JsonW::ARRAY || jreborn->size() != 3)
    {
        LOG_E(tag_) << "Failed to get reborn from " << global_config_filename_;
        return false;
    }
            
    
    pos.set( (uint_fast32_t) jreborn->get(0)->integer(),
                (uint_fast32_t) jreborn->get(1)->integer(),
                (uint_fast32_t) jreborn->get(2)->integer());
                
    if ( cubes_.find( pos ) == cubes_.end() ) 
    {
        LOG_E(tag_) << "reborn position " << pos.x() << "," << pos.y() << "," << pos.z() << " does not exist";
        return false;
    }        

    reborn_ = cubes_.at( pos );

	// read global links
	size_t global_link_count = 0;
    std::shared_ptr<JsonW> jglinks = jglobal->get(u8"links");
    size_t jglinks_size = jglinks == nullptr ? 0 : jglinks->size();
    
    for (size_t idx = 0; idx < jglinks_size; idx++)
    {        
        std::shared_ptr<JsonW> jglink = jglinks->get(idx);
        std::shared_ptr<JsonW> jfrom = jglink->get(u8"from");
        std::shared_ptr<JsonW> jto = jglink->get(u8"to");
        std::shared_ptr<JsonW> jfromcube = jfrom->get("cube");
        std::shared_ptr<JsonW> jtocube = jto->get("cube");

        std::shared_ptr<Cube> from, to;

        // get link type, area 'from' id, area 'to' id
        std::string linktype = jglink->get(u8"type")->str();
		bool is_twoway = true;

		if (linktype == "1way")
		{
			is_twoway = false;
		}

        // read 'from' and 'to'
        // sample: "from": { "area": 1, "cube":"central" },
        // sample: "from": { "cube":[10001,10001,10001] },
        // sample: "from": { "cube":[1,1,1], "offset": [100000, 100000, 100000],: },
        from = readloc( jfrom, marks, cubes_);
        to = readloc( jto, marks, cubes_);

        if (from == nullptr || to == nullptr)
        {
            LOG_E(tag_) << "bad loc in json " << jglink->text();
            continue;
        }
		
		std::shared_ptr<JsonW> jattrs = jglink->get("attr");
		uint_fast32_t attr;
		bool ret;
		if (octillion::Cube::json2attr(jattrs, attr) != OcError::E_SUCCESS)
		{
			ret = octillion::Area::addlink(is_twoway, from, to);
		}
		else
		{
			ret = octillion::Area::addlink(is_twoway, from, to, attr);
		}

        // create link
        if (ret == false)
        {
            LOG_E(tag_) << "failed to add link between " 
                << from->loc().str() << " " << to->loc().str();
        }
        else
        {
            global_link_count++;
        }
    }
    
    // add areaid in each cube
    for ( auto itarea = areas_.begin(); itarea != areas_.end(); itarea ++ )
    {
        std::shared_ptr<octillion::Area> area = (*itarea).second;

        for ( auto it = area->cubes_.begin(); it != area->cubes_.end(); it ++ )
        {
            octillion::CubePosition pos = it->first;
        }
    }
    
    for ( auto itarea = areas_.begin(); itarea != areas_.end(); itarea ++ )
    {
        std::shared_ptr<octillion::Area> area = (*itarea).second;

        for ( auto it = area->cubes_.begin(); it != area->cubes_.end(); it ++ )
        {
            it->second->areaid_ = area->id();
        }
    }
    
    initialized_ = true;
   
    return true;
}

void octillion::WorldMap::dump()
{   
    // area data
    LOG_I(tag_) << "=== Areas Data ===";

    for ( auto itarea = areas_.begin(); itarea != areas_.end(); itarea ++ )
    {
        std::shared_ptr<octillion::Area> area = (*itarea).second;
        
        LOG_I(tag_) << "[" << area->title() << "]";

        for ( auto it = area->cubes_.begin(); it != area->cubes_.end(); it ++ )
        {
            octillion::CubePosition pos = it->first;
            LOG_I(tag_) << pos.str() << it->second->title() << "(area:"
                << it->second->area() << ") "            
                << ( it->second->exits_[0] > 0 ? "N" : "_" )
                << ( it->second->exits_[1] > 0 ? "E" : "_" )
                << ( it->second->exits_[2] > 0 ? "S" : "_" )
                << ( it->second->exits_[3] > 0 ? "W" : "_" )
                << ( it->second->exits_[4] > 0 ? "U" : "_" )
                << ( it->second->exits_[5] > 0 ? "D" : "_" );
        }
    }
    
    // cube data
    LOG_I(tag_) << "=== Cubes Data ===";
    
    for ( auto it = cubes_.begin(); it != cubes_.end(); it ++ )
    {
        octillion::CubePosition pos = it->first;
        LOG_I(tag_) << pos.str() << it->second->title() << "(area:"
                << it->second->area() << ") "            
                << ( it->second->exits_[0] > 0 ? "N" : "_" )
                << ( it->second->exits_[1] > 0 ? "E" : "_" )
                << ( it->second->exits_[2] > 0 ? "S" : "_" )
                << ( it->second->exits_[3] > 0 ? "W" : "_" )
                << ( it->second->exits_[4] > 0 ? "U" : "_" )
                << ( it->second->exits_[5] > 0 ? "D" : "_" );
    }
    
    // other information
    LOG_I(tag_) << "=== Config === ";
    LOG_I(tag_) << "reborn: " << reborn_->loc().str();
    LOG_I(tag_) << "stamp: " << global_config_stamp_;
    
    LOG_I(tag_) << "=== End ===";
    
    return;
}