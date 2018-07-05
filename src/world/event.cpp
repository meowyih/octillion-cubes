
#include "jsonw/jsonw.hpp"
#include "world/event.hpp"

octillion::JsonObjectW* octillion::Event::json()
{
    JsonObjectW* jobject = new JsonObjectW();

    jobject->add("type", type_);

    switch (type_)
    {        
    case TYPE_PLAYER_LEAVE:
        jobject->add("cube", tocube_->json( TYPE_JSON_SIMPLE ));
    case TYPE_PLAYER_LOGOUT:
        jobject->add("player", player_->json( TYPE_JSON_SIMPLE ));        
        break;

    case TYPE_PLAYER_ARRIVE:
        jobject->add("cube", fromcube_->json( TYPE_JSON_SIMPLE ));
    case TYPE_PLAYER_LOGIN:    
        jobject->add("player", player_->json( TYPE_JSON_SIMPLE ));        
        break;
    }

    return jobject;
}