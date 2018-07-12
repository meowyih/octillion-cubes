
#include "jsonw/jsonw.hpp"
#include "world/event.hpp"

octillion::JsonW* octillion::Event::json()
{
    JsonW* jobject = new JsonW();

    jobject->add("type", type_);

    switch (type_)
    {        
    case TYPE_PLAYER_LEAVE:
        jobject->add("subcube", subcube_->json( TYPE_JSON_SIMPLE ));
        jobject->add("dir", direction_);
    case TYPE_PLAYER_LOGOUT:
        jobject->add("player", player_.json( TYPE_JSON_SIMPLE ));
        break;

    case TYPE_PLAYER_ARRIVE:
        jobject->add("subcube", subcube_->json( TYPE_JSON_SIMPLE ));
        jobject->add("dir", direction_);
    case TYPE_PLAYER_LOGIN:    
        jobject->add("player", player_.json( TYPE_JSON_SIMPLE ));
        break;
    }

    return jobject;
}