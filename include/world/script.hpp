#ifndef OCTILLION_SCRIPT_HEADER
#define OCTILLION_SCRIPT_HEADER

#include <vector>

#include "jsonw/jsonw.hpp"
#include "world/storage.hpp"
#include "world/cubeposition.hpp"

namespace octillion
{
    class Script;
    class Action;
    class Condition;
}

class octillion::Action
{
private:
    const std::string tag_ = "Action";

public:
    const static int ACTION_TYPE_UNKNOWN = 0;
    const static int ACTION_TYPE_SET = 1;
    const static int ACTION_TYPE_TEXT = 2;
    const static int ACTION_TYPE_RESET_TIMER = 3;

    const static int VARIABLE_TYPE_UNKNOWN = 0;
    const static int VARIABLE_TYPE_LOCATION = 10;
    const static int VARIABLE_TYPE_TEXT = 11;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_1 = 100;
    const static int VARIABLE_TYPE_PERMANENT_INTEGER_1 = 201;

public:
    Action();
    bool init(std::shared_ptr<JsonW> json,
        std::map<std::string, CubePosition>& markmap,
        uint_fast32_t offset_x,
        uint_fast32_t offset_y,
        uint_fast32_t offset_z);

public:
    int action_type_;
    int variable_type_;

    bool b_rhs_;
    int  i_rhs_;
};

class octillion::Condition
{
private:
    const std::string tag_ = "Condition";

public:
    const static int CONDITION_TYPE_UNKNOWN = 0;
    const static int CONDITION_TYPE_EQUAL = 1;
    const static int CONDITION_TYPE_NOT_EQUAL = 2;
    const static int CONDITION_TYPE_GREATER = 3;
    
    const static int VARIABLE_TYPE_UNKNOWN = 0;
    const static int VARIABLE_TYPE_LOCATION            = 10;

    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_1 = 101;
    /*
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_2 = 102;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_3 = 103;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_4 = 104;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_5 = 105;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_6 = 106;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_7 = 107;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_8 = 108;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_9 = 109;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_10 = 110;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_11 = 111;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_12 = 112;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_13 = 113;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_14 = 114;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_15 = 115;
    const static int VARIABLE_TYPE_PERMANENT_BOOLEAN_16 = 116;
    */
    const static int VARIABLE_TYPE_PERMANENT_INTEGER_1 = 201;
    
    const static int VARIABLE_TYPE_MOVE_COUNT = 1000;

    const static int VARIABLE_TYPE_TIMER = 2000;
public:
    Condition();
    bool init(std::shared_ptr<JsonW> json,
        std::map<std::string, CubePosition>& markmap,
        uint_fast32_t offset_x,
        uint_fast32_t offset_y,
        uint_fast32_t offset_z);
    bool is_true(
        octillion::Storage& var, bool is_moving );

    bool is_true_location(octillion::Storage& var);
    bool is_true_boolean(octillion::Storage& var);
    bool is_true_move_count(octillion::Storage& var);
    bool is_true_timer(octillion::Storage& var);
    bool is_true_integer(octillion::Storage& var);

public:
    int condition_type_;
    int variable_type_;

    bool b_rhs_;
    int  i_rhs_;
    octillion::CubePosition loc_rhs_;
};

class octillion::Script
{
private:
    const std::string tag_ = "Script";

public:
    const static int TYPE_CONDITION_NONE = 0;
    const static int TYPE_CONDITION_AND = 1;
    const static int TYPE_CONDITION_OR  = 2;
    const static int TYPE_CONDITION_NOT = 3;

public:
    Script();

    bool init(std::shared_ptr<JsonW> json, 
        std::map<std::string, CubePosition>& markmap,
        uint_fast32_t offset_x, 
        uint_fast32_t offset_y, 
        uint_fast32_t offset_z);

    // called when player arrive one cube, return nullptr if no available action
    std::shared_ptr<std::vector<octillion::Action>> 
        handle( octillion::Storage& var, bool is_moving );

private:
    // help function to determine if condition is true
    inline bool condition_and(octillion::Storage& var, bool is_moving);
    inline bool condition_or(octillion::Storage& var, bool is_moving);
    inline bool condition_not(octillion::Storage& var, bool is_moving);
private:
    int condition_type_;
    std::vector<octillion::Condition> conditions_; 
    std::shared_ptr<std::vector<octillion::Action>> p_actions_move_;
};

#endif