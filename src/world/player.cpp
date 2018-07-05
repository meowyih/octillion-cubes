
#include <ostream>
#include <sstream>
#include <string>
#include <cstdlib>

#include "world/cube.hpp"
#include "world/player.hpp"
#include "world/event.hpp"

#include "jsonw/jsonw.hpp"

// parameter type
// 1 - simple, for login/logout/arrive/leave event usage
// 2 - detail, for see event usage
octillion::JsonObjectW* octillion::Player::json( int type )
{
    JsonObjectW* jobject = new JsonObjectW();
    switch (type)
    {
    case Event::TYPE_JSON_DETAIL:
        jobject->add("con", (int)con_);
        jobject->add("men", (int)men_);
        jobject->add("luc", (int)luc_);
        jobject->add("cha", (int)cha_);
        jobject->add("status", (int)status_);

    case Event::TYPE_JSON_SIMPLE:
        jobject->add("id", (int)id_);
        jobject->add("gender", (int)gender_);
        jobject->add("cls", (int)cls_);
        break;
    }

    return jobject;
}