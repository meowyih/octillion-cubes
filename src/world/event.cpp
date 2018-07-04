
#include "jsonw/jsonw.hpp"
#include "world/event.hpp"

octillion::JsonObjectW* octillion::Event::json()
{
    JsonObjectW* jobject = new JsonObjectW();

    jobject->add("type", type_);

    return jobject;
}