#include <chrono>

#include "error/macrolog.hpp"
#include "jsonw/jsonw.hpp"
#include "world/script.hpp"
#include "world/cubeposition.hpp"

octillion::Script::Script()
{
    condition_type_ = TYPE_CONDITION_NONE;
    p_actions_move_ = std::make_shared<std::vector<octillion::Action>>();
}

bool octillion::Script::init(std::shared_ptr<JsonW> json,
    std::map<std::string, CubePosition>& markmap,
    uint_fast32_t offset_x,
    uint_fast32_t offset_y,
    uint_fast32_t offset_z)
{
    // check condition is one of and / or / not
    std::shared_ptr<JsonW> json_and, json_or, json_not;
    std::shared_ptr<JsonW> json_actions, json_conditions;

    json_and = json->get(u8"and");
    json_or = json->get(u8"or");
    json_not = json->get(u8"not");

    if (json_and == nullptr && json_or == nullptr && json_not == nullptr)
    {
        LOG_E(tag_) << "script needs one and only one and/or/not condition";
        return false;
    }

    if (json_and != nullptr)
    {
        json_conditions = json_and;
        condition_type_ = TYPE_CONDITION_AND;
    }

    if (json_or != nullptr)
    {
        if (condition_type_ != TYPE_CONDITION_NONE)
        {
            LOG_E(tag_) << "script needs one and only one and/or/not condition";
            return false;
        }

        json_conditions = json_or;
        condition_type_ = TYPE_CONDITION_OR;
    }

    if (json_not != nullptr)
    {
        if (condition_type_ != TYPE_CONDITION_NONE)
        {
            LOG_E(tag_) << "script needs one and only one and/or/not condition";
            return false;
        }

        json_conditions = json_not;
        condition_type_ = TYPE_CONDITION_NOT;
    }

    if (json_conditions->type() != JsonW::ARRAY)
    {
        LOG_E(tag_) << "script condition section need to be array type";
        return false;
    }

    // parse conditions
    for (size_t i = 0; i < json_conditions->size(); i++)
    {
        octillion::Condition cond;
        std::shared_ptr<JsonW> json_cond = json_conditions->get(i);
        if (!cond.init(json_cond, markmap, offset_x, offset_y, offset_z))
        {
            LOG_E(tag_) << "script condition init failed";
            return false;
        }
        conditions_.push_back(cond);
    }

    // parse actions
    json_actions = json->get(u8"actions");

    if (json_actions->type() != JsonW::ARRAY)
    {
        LOG_E(tag_) << "script actions section need to be array type";
        return false;
    }

    for (size_t i = 0; i < json_actions->size(); i++)
    {
        octillion::Action act;
        std::shared_ptr<JsonW> json_act = json_actions->get(i);
        if (!act.init(json_act, markmap, offset_x, offset_y, offset_z))
        {
            LOG_E(tag_) << "script condition init failed";
            return false;
        }
        p_actions_move_->push_back(act);
    }

    return true;
}

std::shared_ptr<std::vector<octillion::Action>>  octillion::Script::handle(octillion::Storage& var, bool is_moving)
{
    bool cond;

    switch (condition_type_)
    {
    case octillion::Script::TYPE_CONDITION_AND:
        cond = condition_and(var, is_moving);
        break;
    case octillion::Script::TYPE_CONDITION_OR:
        cond = condition_or(var, is_moving);
        break;
    case octillion::Script::TYPE_CONDITION_NOT:
        cond = condition_not(var, is_moving);
        break;
    default:
        cond = false;
    }

    if (cond == false)
        return nullptr;

    return p_actions_move_;
}

inline bool octillion::Script::condition_and(octillion::Storage& var, bool is_moving)
{
    bool result = true;
    for (auto it = conditions_.begin(); it != conditions_.end(); it++)
    {
        if (!(*it).is_true(var, is_moving))
        {
            result = false;
            break;
        }
    }
    return result;
}

inline bool octillion::Script::condition_or(octillion::Storage& var, bool is_moving)
{
    bool result = false;
    for (auto it = conditions_.begin(); it != conditions_.end(); it++)
    {
        if ((*it).is_true(var, is_moving))
        {
            result = true;
            break;
        }
    }
    return result;
}

inline bool octillion::Script::condition_not(octillion::Storage& var, bool is_moving)
{    
    return ! condition_and( var, is_moving);
}

octillion::Action::Action()
{
    action_type_ = ACTION_TYPE_UNKNOWN;
    variable_type_ = VARIABLE_TYPE_UNKNOWN;

    b_rhs_ = false;
    i_rhs_ = 0;
}

bool octillion::Action::init(std::shared_ptr<JsonW> json,
    std::map<std::string, CubePosition>& markmap,
    uint_fast32_t offset_x,
    uint_fast32_t offset_y,
    uint_fast32_t offset_z )
{
    std::shared_ptr<JsonW> jtype = json->get(u8"type");
    std::shared_ptr<JsonW> jvar;

    // check type
    if (jtype == nullptr || jtype->type() != JsonW::STRING)
    {
        LOG_E(tag_) << "script condition must has a string type";
        return false;
    }
    else if (jtype->str() == u8"set")
    {
        action_type_ = ACTION_TYPE_SET;
    }
    else if (jtype->str() == u8"text")
    {
        action_type_ = ACTION_TYPE_TEXT;
    }
    else if (jtype->str() == u8"reset timer")
    {
        action_type_ = ACTION_TYPE_RESET_TIMER;
        return true; // reset timer does not have variable
    }
    else
    {
        LOG_E(tag_) << "script condition does not support type: " << jtype->str();
        return false;
    }

    // check variable
    if ((jvar = json->get(u8"b01")) != nullptr && jvar->type() == JsonW::BOOLEAN)
    {
        variable_type_ = VARIABLE_TYPE_PERMANENT_BOOLEAN_1;
        b_rhs_ = jvar->boolean();
        return true;
    }

    if ((jvar = json->get(u8"i01")) != nullptr && jvar->type() == JsonW::INTEGER)
    {
        variable_type_ = VARIABLE_TYPE_PERMANENT_INTEGER_1;
        i_rhs_ = (int)(jvar->integer());
        return true;
    }

    if ((jvar = json->get(u8"string")) != nullptr && jvar->type() == JsonW::INTEGER)
    {
        variable_type_ = VARIABLE_TYPE_TEXT;
        i_rhs_ = (int)(jvar->integer());
        return true;
    }

    LOG_E(tag_) << "script action does not have valid variable type:" << json->text();
    return false;
}

octillion::Condition::Condition()
{
    condition_type_ = CONDITION_TYPE_UNKNOWN;
    variable_type_ = VARIABLE_TYPE_UNKNOWN;

    b_rhs_ = false;
    i_rhs_ = 0;
}

bool octillion::Condition::init(std::shared_ptr<JsonW> json,
    std::map<std::string, CubePosition>& markmap,
    uint_fast32_t offset_x,
    uint_fast32_t offset_y,
    uint_fast32_t offset_z )
{
    std::shared_ptr<JsonW> jtype = json->get(u8"type");
    std::shared_ptr<JsonW> jvar;

    // check type
    if (jtype == nullptr || jtype->type() != JsonW::STRING)
    {
        LOG_E(tag_) << "script condition must has a string type";
        return false;
    }
    else if (jtype->str() == u8"equal")
    {
        condition_type_ = CONDITION_TYPE_EQUAL;
    }
    else if (jtype->str() == u8"not equal")
    {
        condition_type_ = CONDITION_TYPE_NOT_EQUAL;
    }
    else if (jtype->str() == u8"greater")
    {
        condition_type_ = CONDITION_TYPE_GREATER;
    }
    else
    {
        LOG_E(tag_) << "script condition does not support type: " << jtype->str();
        return false;
    }

    // check variable
    if ((jvar = json->get(u8"b01")) != nullptr && jvar->type() == JsonW::BOOLEAN )
    {
        variable_type_ = VARIABLE_TYPE_PERMANENT_BOOLEAN_1;
        b_rhs_ = jvar->boolean();
        return true;
    }

    if ((jvar = json->get(u8"i01")) != nullptr && jvar->type() == JsonW::INTEGER)
    {
        variable_type_ = VARIABLE_TYPE_PERMANENT_INTEGER_1;
        b_rhs_ = jvar->integer();
        return true;
    }

    if ((jvar = json->get(u8"loc")) != nullptr 
        && jvar->type() == JsonW::ARRAY 
        && jvar->size() == 3 
        && jvar->get(0)->type() == JsonW::INTEGER
        && jvar->get(1)->type() == JsonW::INTEGER
        && jvar->get(2)->type() == JsonW::INTEGER )
    {
        variable_type_ = VARIABLE_TYPE_LOCATION;        
        loc_rhs_.set(
            (uint_fast32_t)jvar->get(0)->integer() + offset_x, 
            (uint_fast32_t)jvar->get(1)->integer() + offset_y,
            (uint_fast32_t)jvar->get(2)->integer() + offset_z);
        return true;
    }

    if ((jvar = json->get(u8"loc")) != nullptr
        && jvar->type() == JsonW::STRING )
    {
        std::string mark = jvar->str();
        auto it = markmap.find(mark);

        if (it == markmap.end())
        {
            LOG_E(tag_) << "script condition detect invalid cube mark " << mark;
            return false;
        }

        variable_type_ = VARIABLE_TYPE_LOCATION;
        loc_rhs_ = it->second;
        return true;
    }

    if ((jvar = json->get(u8"move count")) != nullptr
        && jvar->type() == JsonW::INTEGER)
    {
        int count = (int)(jvar->integer());
        variable_type_ = VARIABLE_TYPE_MOVE_COUNT;
        i_rhs_ = count;
        return true;
    }

    if ((jvar = json->get(u8"timer")) != nullptr
        && jvar->type() == JsonW::INTEGER)
    {
        int count = (int)(jvar->integer());
        variable_type_ = VARIABLE_TYPE_TIMER;
        i_rhs_ = count;
        return true;
    }

    LOG_E(tag_) << "script condition does not have valid type:" << json->text();
    return false;
}

bool octillion::Condition::is_true(octillion::Storage& var, bool is_moving )
{
    switch (variable_type_)
    {
    case VARIABLE_TYPE_LOCATION:
        if (is_moving)
            return is_true_location(var);
        else
            return false;
    case VARIABLE_TYPE_PERMANENT_BOOLEAN_1:
        return is_true_boolean(var);
    case VARIABLE_TYPE_PERMANENT_INTEGER_1:
        return is_true_integer(var);
    case VARIABLE_TYPE_MOVE_COUNT:
        if (is_moving)
            return is_true_move_count(var);
        else
            return false;
    case VARIABLE_TYPE_TIMER:
        return is_true_timer(var);
    }

    LOG_E(tag_) << "is_true() Unknown variable type: " << variable_type_;
    return false;
}

bool octillion::Condition::is_true_location(octillion::Storage& var)
{
    switch (condition_type_)
    {
    case CONDITION_TYPE_EQUAL:
        return ( loc_rhs_ == var.loc_ );
    case CONDITION_TYPE_NOT_EQUAL:
        return !(loc_rhs_ == var.loc_);
    }

    LOG_E(tag_) << "is_true_location(), unknown condition type:" << variable_type_;
    return false;
}

bool octillion::Condition::is_true_boolean(octillion::Storage& var)
{
    int index = 0;
    bool b_rhs;
    switch(variable_type_)
    {
        case VARIABLE_TYPE_PERMANENT_BOOLEAN_1: index = 0; break;
        default:
            LOG_E(tag_) << "is_true_boolean(), unknown variable type:" << variable_type_;
            return false;
    }

    b_rhs = var.boolean(var.areaid_, index);

    switch (condition_type_)
    {
    case CONDITION_TYPE_EQUAL:
        return true == b_rhs_;
    case CONDITION_TYPE_NOT_EQUAL:
        return false == b_rhs_;
    }

    LOG_E(tag_) << "is_true_boolean(), unknown condition type:" << variable_type_;
    return false;
}

bool octillion::Condition::is_true_move_count(octillion::Storage& var)
{
    switch (condition_type_)
    {
    case CONDITION_TYPE_EQUAL:
        return (var.move_count(var.areaid_) == i_rhs_);
    case CONDITION_TYPE_NOT_EQUAL:
        return !(var.move_count(var.areaid_) == i_rhs_);
    case CONDITION_TYPE_GREATER:
        return ((int)(var.move_count(var.areaid_)) > i_rhs_);
    }

    LOG_E(tag_) << "is_true_move_count(), unknown condition type:" << variable_type_;
    return false;
}

bool octillion::Condition::is_true_timer(octillion::Storage& var)
{
    std::chrono::duration<double, std::milli> diff =
        std::chrono::high_resolution_clock::now() - var.timer_;

    switch (condition_type_)
    {
    case CONDITION_TYPE_GREATER:
        return (diff.count() / 1000 > i_rhs_);
    }

    LOG_E(tag_) << "is_true_timer(), only support CONDITION_TYPE_GREATER, type:" << variable_type_ << " is invalid";
    return false;
}

bool octillion::Condition::is_true_integer(octillion::Storage& var)
{
    int index = 0;
    int i_rhs;
    switch (variable_type_)
    {
    case VARIABLE_TYPE_PERMANENT_INTEGER_1: index = 0; break;
    default:
        LOG_E(tag_) << "is_true_integer(), unknown variable type:" << variable_type_;
        return false;
    }

    i_rhs = var.integer(var.areaid_, index);

    switch (condition_type_)
    {
    case CONDITION_TYPE_EQUAL:
        return i_rhs == i_rhs_;
    case CONDITION_TYPE_NOT_EQUAL:
        return i_rhs != i_rhs_;
    }

    LOG_E(tag_) << "is_true_integer(), unknown condition type:" << variable_type_;
    return false;
}
