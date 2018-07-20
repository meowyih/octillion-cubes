#ifndef OCTILLION_CREATURE_HEADER
#define OCTILLION_CREATURE_HEADER

#include <cstdint>
#include "jsonw/jsonw.hpp"

namespace octillion
{
	class Creature;
}

class octillion::Creature
{
public:
	virtual JsonW* json(uint_fast32_t type) = 0;

	int_fast32_t hp() { return hp_; }
	int_fast32_t maxhp() { return maxhp_; }
	void hp(int_fast32_t hp) { hp_ = hp; }
	void maxhp(int_fast32_t maxhp) { maxhp_ = maxhp; }

protected:
	int_fast32_t hp_ = 10;
	int_fast32_t maxhp_ = 10;
};

#endif