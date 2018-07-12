
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
octillion::JsonW* octillion::Player::json( int type )
{
    JsonW* jobject = new JsonW();
    switch (type)
    {
    case Event::TYPE_JSON_DETAIL:
        jobject->add("con", con_);
        jobject->add("men", men_);
        jobject->add("luc", luc_);
        jobject->add("cha", cha_);
        jobject->add("status", status_);

    case Event::TYPE_JSON_SIMPLE:
        jobject->add("id", (int)id_);
        jobject->add("gender", gender_);
        jobject->add("cls", cls_);
        break;
    }

    return jobject;
}