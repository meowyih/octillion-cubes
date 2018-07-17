
#include "jsonw/jsonw.hpp"
#include "world/event.hpp"

octillion::JsonW* octillion::Event::json()
{
    JsonW* jobject = new JsonW();

    jobject->add("type", type_);

    switch (type_)
    {        
    case TYPE_PLAYER_LEAVE:
		jobject->add("player", player_.json(TYPE_JSON_SIMPLE));
        jobject->add("subcube", subcube_->json( TYPE_JSON_SIMPLE ));
        jobject->add("dir", direction_);
		break;

    case TYPE_PLAYER_LOGOUT:
        jobject->add("player", player_.json( TYPE_JSON_SIMPLE_WITH_LOC ));
        break;

    case TYPE_PLAYER_ARRIVE:
		jobject->add("player", player_.json(TYPE_JSON_SIMPLE));
        jobject->add("subcube", subcube_->json( TYPE_JSON_SIMPLE ));
        jobject->add("dir", direction_);
		break;

    case TYPE_PLAYER_LOGIN:    
        jobject->add("player", player_.json( TYPE_JSON_SIMPLE_WITH_LOC ));
        break;

	case TYPE_PLAYER_DEAD:
		jobject->add("player", player_.json(TYPE_JSON_SIMPLE_WITH_LOC));
		break;

	case TYPE_PLAYER_REBORN:
		jobject->add("player", player_.json(TYPE_JSON_SIMPLE_WITH_LOC));
		break;

	case TYPE_MOB_ATTACK:
		jobject->add("mob", mob_->json(TYPE_JSON_SIMPLE));
		jobject->add("dmg", i32parm);
		break;

	case TYPE_MOB_ARRIVE:
		jobject->add("mob", mob_->json(TYPE_JSON_SIMPLE));
		jobject->add("subcube", subcube_->json(TYPE_JSON_SIMPLE));
		jobject->add("dir", Cube::dir(eventcube_, subcube_));
		break;

	case TYPE_MOB_LEAVE:
		jobject->add("mob", mob_->json(TYPE_JSON_SIMPLE));
		jobject->add("subcube", subcube_->json(TYPE_JSON_SIMPLE));
		jobject->add("dir", Cube::dir( eventcube_, subcube_ ));
		break;

	case TYPE_MOB_DEAD:
		jobject->add("mob", mob_->json(TYPE_JSON_SIMPLE));
		break;
	case TYPE_MOB_REBORN:
		jobject->add("mob", mob_->json(TYPE_JSON_SIMPLE));
		break;
    }

    return jobject;
}