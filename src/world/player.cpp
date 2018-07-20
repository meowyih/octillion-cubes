
#include <ostream>
#include <sstream>
#include <string>
#include <cstdlib>

#include "world/cube.hpp"
#include "world/player.hpp"
#include "world/event.hpp"

#include "jsonw/jsonw.hpp"

void octillion::Player::status(int status)
{
	if (status_ == status)
		return;

	if (status_ == STATUS_IDLE)
	{
		if (status == STATUS_COMBAT)
		{
			// fight!
		}
		else
		{
			LOG_W(tag_) << "status(), missing " << status_ << "->" << status << " handler";
		}
	}
	else if (status_ = STATUS_COMBAT)
	{
		if (status == STATUS_IDLE)
		{
			target_ = NULL;
		}
		else
		{
			LOG_W(tag_) << "status(), missing " << status_ << "->" << status << " handler";
		}
	}
	else
	{
		LOG_W(tag_) << "status() set unknown status:" << status;
		return;
	}

	status_ = status;
}

octillion::JsonW* octillion::Player::json(uint_fast32_t type )
{
    JsonW* jobject = new JsonW();

	jobject->add("id", (int)id_);
	jobject->add("status", status_);

	if ((type & J_HP) != 0)
	{
		JsonW* jarray = new JsonW();
		jarray->add((int)hp());
		jarray->add((int)maxhp());
		jobject->add(u8"hp", jarray);
	}

	if ((type & J_CUBE) != 0)
	{
		jobject->add("loc", cube_->json(0));
	}

	if ((type & J_ATTR) != 0)
	{
		jobject->add("gender", gender_);
		jobject->add("cls", cls_);
		jobject->add("con", con_);
		jobject->add("men", men_);
		jobject->add("luc", luc_);
		jobject->add("cha", cha_);
	}

	if ((type & J_DISPLAY) != 0)
	{
		jobject->add("name", username_);
	}

    return jobject;
}