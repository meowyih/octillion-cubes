
#include <string>
#include <sstream>
#include <system_error>
#include <map>
#include <list>

#include "error/macrolog.hpp"

#include "jsonw/jsonw.hpp"

#include "world/mob.hpp"
#include "world/cube.hpp"
#include "world/world.hpp"

octillion::Mob::Mob(const JsonW* jmob)
{
	valid_ = false;

	if (jmob == NULL || jmob->type() != JsonW::OBJECT)
	{
		LOG_E(tag_) << "jmob is not object";
		return;
	}

	JsonW *jlevel, *jrace, *jsubrace, *jctype;
	JsonW *jshort, *jlong;

	Mob* mob_default = NULL;

	jlevel = jmob->get(u8"level");
	jrace = jmob->get(u8"race");
	jsubrace = jmob->get(u8"subrace");
	jctype = jmob->get(u8"combat_type");

	jshort = jmob->get(u8"short");
	jlong = jmob->get(u8"long");

	if (jlevel == NULL || jlevel->type() != JsonW::INTEGER ||
		jrace == NULL || jrace->type() != JsonW::INTEGER )
	{
		LOG_E(tag_) << "jlevel or jrace invalid";
		return;
	}

	lvl_ = (int)jlevel->integer();
	race_ = (int)jrace->integer();

	if (jsubrace != NULL)
	{
		subrace_ = (int) jsubrace->integer();
	}

	if (jctype != NULL)
	{
		combat_ = (int)jctype->integer();
	}

	if (jshort != NULL)
	{
		short_ = jshort->str();
	}
	else
	{
		std::stringstream ss;
		ss << "mob-" << race_ << "-" << subrace_ << "-" << lvl_;
		short_ = ss.str();
	}

	if (jlong != NULL)
	{
		long_ = jlong->str();
	}
	else
	{
		long_ = short_;
	}

	valid_ = true;
}
/*
"mobs":
	[
		{
			"id": 1,

			"level": 12,
			"race": 2,
			"subrace": 1,

			"combat_type": 42,
			"short": "test mob#1",
			"long": "A ghost like monster flies in the air",

			"random": false,
			"loc": [1,4,4],			
			"movetick": [5, 10],
			"path":
			[
				"contral room",
				[1,4,4],
				[4,7,7],
				"underground-12"
			]
	}
]

// read 'paths'
// sample: "from": { "area": 1, "cube":"central" },
// sample: "from": { "cube":[10001,10001,10001] },
// sample: "from": { "cube":[1,1,1], "offset": [100000, 100000, 100000], },
// has no move path but has move tick means random
*/
octillion::Mob::Mob(
	int areaid, int order,
	const JsonW* jmob,
	const std::map<uint_fast32_t, Mob*>& default,
	const std::map<int, std::map<std::string, Cube*>*>& area_marks,
	const std::map<CubePosition, Cube*>& cubes,
	CubePosition goffset
	)
{
	valid_ = false;
	if (jmob == NULL || jmob->type() != JsonW::OBJECT)
	{
		LOG_E(tag_) << " bad json with no object";
		return;
	}

	JsonW *jlevel, *jrace, *jsubrace, *jctype;
	JsonW *jshort, *jlong;
	JsonW *jloc, *jpath, *jmtick, *jrandom;

	Mob* mob_default = NULL;

	jlevel = jmob->get(u8"level");
	jrace = jmob->get(u8"race");
	jsubrace = jmob->get(u8"subrace");
	jctype = jmob->get(u8"combat_type");

	jshort = jmob->get(u8"short");
	jlong = jmob->get(u8"long");

	jloc = jmob->get(u8"loc");
	jmtick = jmob->get(u8"movetick");
	jpath = jmob->get(u8"path");
	jrandom = jmob->get(u8"random");

	if (jlevel == NULL || jlevel->type() != JsonW::INTEGER || 
		jrace == NULL || jrace->type() != JsonW::INTEGER || 
		jloc == NULL )
	{
		LOG_E(tag_) << " bad json without id/level/race/loc";
		return;
	}

	// 0x0001 0002 - area 1, local id 2 
	area_ = areaid;
	id_ = order;
	lvl_ = (int)jlevel->integer();
	race_ = (int)jrace->integer();

	if (jsubrace != NULL)
	{
		subrace_ = (int)jsubrace->integer();
	}	

	auto itmob = default.find(type());
	if (itmob != default.end())
	{
		mob_default = itmob->second;
	}

	if (jctype != NULL)
	{
		combat_ = (int)jctype->integer();
	}

	if (jshort != NULL)
	{
		short_ = jshort->str();
	}
	else if ( mob_default != NULL )
	{
		short_ = mob_default->short_;
	}
	else
	{
		std::stringstream ss;
		ss << "mob-" << race_ << "-" << subrace_ << "-" << lvl_;
		short_ = ss.str();
	}

	if (jlong != NULL)
	{
		long_ = jlong->str();
	}
	else if (mob_default != NULL)
	{
		long_ = mob_default->long_;
	}
	else
	{
		long_ = short_;
	}

	loc_ = readloc(areaid, jloc, area_marks, cubes, goffset);
	if (loc_ == NULL)
	{
		LOG_E(tag_) << " bad json without valid loc " << jloc->text();
		return;
	}

	if (jmtick != NULL && jmtick->type() == JsonW::ARRAY && jmtick->size() == 2)
	{
		min_move_tick_ = (unsigned int) jmtick->get(0)->integer();
		max_move_tick_ = (unsigned int) jmtick->get(1)->integer();
		if (max_move_tick_ < min_move_tick_)
		{
			unsigned int tmp;
			tmp = max_move_tick_;
			max_move_tick_ = min_move_tick_;
			min_move_tick_ = tmp;
		}

		next_move_count_ = max_move_tick_;
	}

	if (jrandom != NULL)
	{
		random_path_ = jrandom->boolean();
	}

	if (jpath != NULL && jpath->type() == JsonW::ARRAY)
	{
		for (size_t idx = 0; idx < jpath->size(); idx++)
		{
			JsonW* jpitem = jpath->get(idx);
			Cube* pcube = readloc(areaid, jpitem, area_marks, cubes, goffset);
			if (pcube == NULL)
			{
				LOG_E(tag_) << " path item " << jpitem->text() << " does not exist";
				return;
			}
			path_.push_back(pcube);
		}
	}

	valid_ = true;

	/*
	{
		"local_id": 1,

		"level" : 12,
		"race" : 2,
		"subrace" : 1,
		"combat_type" : 42,

		"short" : "test mob#1",
		"long" : "A ghost like monster flies in the air",

		"loc" : [1, 4, 4],
		"movetick" : [5, 10],
		"path" :
		[
			"contral room",
			[1, 4, 4],
			[4, 7, 7],
			"underground-12"
		]
	}
	*/
}

octillion::Cube* octillion::Mob::next_path()
{
	if (path_.size() == 0)
	{
		return NULL;
	}

	// move via path
	path_idx_++;
	if (path_idx_ >= (int) path_.size())
	{
		path_idx_ = 0;
	}

	return path_.at(path_idx_);
}

uint_fast32_t octillion::Mob::id()
{
	uint_fast32_t areaid = area_;
	uint_fast32_t id = id_;
	return areaid << 16 | id;
}

uint_fast32_t octillion::Mob::type()
{
	uint_fast32_t subrace = subrace_;
	uint_fast32_t race = race_;
	return subrace << 16 | race;
}
octillion::Cube* octillion::Mob::readloc(
	int areaid,
	const JsonW* jloc,
	const std::map<int, std::map<std::string, Cube*>*>& area_marks,
	const std::map<CubePosition, Cube*>& cubes,
	CubePosition goffset)
{
	Cube* loc = NULL;
	if (jloc->type() == JsonW::STRING)
	{
		std::map<std::string, Cube*>* marks;
		auto itmarks = area_marks.find(areaid);
		if (itmarks == area_marks.end() || itmarks->second == NULL)
		{
			return loc;
		}
		marks = itmarks->second;
		auto itcube = marks->find(jloc->str());
		if (itcube == marks->end())
		{
			return loc;
		}
		loc = itcube->second;
	}
	else if (jloc->type() == JsonW::ARRAY)
	{
		CubePosition pos;
		bool ret = Area::readloc(jloc, pos, goffset.x(), goffset.y(), goffset.z());
		if (ret == false)
		{
			return loc;
		}

		auto itcube = cubes.find(pos);
		if (itcube == cubes.end())
		{
			return loc;
		}
		loc = itcube->second;
	}

	return loc;
}

octillion::JsonW* octillion::Mob::json(int type)
{
	JsonW* jobject = new JsonW();
	switch (type)
	{
	case Event::TYPE_JSON_DETAIL:
	case Event::TYPE_JSON_SIMPLE:
		jobject->add("id", id_);
		jobject->add("area", area_);
		jobject->add("hp", (long long)hp_);
		break;
	}

	return jobject;
}