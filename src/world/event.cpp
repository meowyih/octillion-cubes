
#include "jsonw/jsonw.hpp"
#include "world/event.hpp"

octillion::JsonW* octillion::Event::json()
{
    JsonW* jobject = new JsonW();

    jobject->add("type", type_);

    switch (type_)
    {        
    case TYPE_PLAYER_LEAVE:
		jobject->add("player", player_->json(Player::J_CUBE));
        jobject->add("subcube", subcube_->json(0));
        jobject->add("dir", direction_);
		break;

    case TYPE_PLAYER_LOGOUT:
        jobject->add("player", player_->json(Player::J_CUBE));
        break;

    case TYPE_PLAYER_ARRIVE:
		jobject->add("player", player_->json(0));
        jobject->add("subcube", subcube_->json(0));
        jobject->add("dir", direction_);
		break;

	case TYPE_PLAYER_ARRIVE_PRIVATE:
		jobject->add("player", player_->json(Player::J_CUBE));
		jobject->add("cube", eventcube_->json(Cube::J_MOB | Cube::J_PLAYER));
		jobject->add("dir", direction_);
		break;

    case TYPE_PLAYER_LOGIN:    
        jobject->add("player", player_->json(0));
		jobject->add("cube", eventcube_->json(0));
        break;

	case TYPE_PLAYER_LOGIN_PRIVATE:
		jobject->add("player", player_->json(Player::J_HP | Player::J_ATTR | Player::J_CUBE));
		jobject->add("cube", eventcube_->json(Cube::J_MOB | Cube::J_PLAYER));
		break;

	case TYPE_PLAYER_DEAD:
		jobject->add("player", player_->json(Player::J_CUBE));
		jobject->add("mob", mob_->json(0));
		break;

	case TYPE_PLAYER_REBORN:
		jobject->add("player", player_->json(0));
		break;

	case TYPE_PLAYER_REBORN_PRIVATE:
		jobject->add("player", player_->json(Player::J_CUBE));
		jobject->add("cube", eventcube_->json(Cube::J_MOB | Cube::J_PLAYER));
		break;

	case TYPE_PLAYER_ATTACK:
		jobject->add("mob", mob_->json(Player::J_HP));
		jobject->add("player", player_->json(0));
		jobject->add("dmg", i32parm_);
		break;

	case TYPE_MOB_REBORN:
		jobject->add("mob", mob_->json(Mob::J_HP | Mob::J_CUBE));
		break;

	case TYPE_MOB_ATTACK:
		jobject->add("mob", mob_->json(0));
		jobject->add("player", player_->json(Player::J_HP));
		jobject->add("dmg", i32parm_);
		break;

	case TYPE_MOB_ARRIVE:
		jobject->add("mob", mob_->json(Mob::J_HP | Mob::J_CUBE));
		jobject->add("subcube", subcube_->json(0));
		jobject->add("dir", Cube::dir(eventcube_, subcube_));
		break;

	case TYPE_MOB_LEAVE:
		jobject->add("mob", mob_->json(Mob::J_CUBE));
		jobject->add("subcube", subcube_->json(0));
		jobject->add("dir", Cube::dir( eventcube_, subcube_ ));
		break;

	case TYPE_MOB_DEAD:
		jobject->add("mob", mob_->json(0));
		jobject->add("player", player_->json(Player::J_HP));
		jobject->add("dmg", i32parm_);
		break;
    }

    return jobject;
}