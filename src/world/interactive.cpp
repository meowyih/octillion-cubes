#include <memory>

#include "jsonw/jsonw.hpp"
#include "world/interactive.hpp"
#include "error/macrolog.hpp"

octillion::Interactive::Interactive()
{
    area_id_ = 0;
    obj_id_ = 0;
}

bool octillion::Interactive::init(int areaid,
    std::shared_ptr<JsonW> json,
    std::map<std::string, octillion::CubePosition>& markmap,
    uint_fast32_t offset_x,
    uint_fast32_t offset_y,
    uint_fast32_t offset_z,
    octillion::StringTable& table)
{
    area_id_ = areaid;

    if (json == nullptr)
    {
        LOG_E(tag_) << "init recv nullptr json";
        return false;
    }

    std::shared_ptr<JsonW> jid = json->get(u8"id");
    std::shared_ptr<JsonW> jtitle = json->get(u8"title");
    std::shared_ptr<JsonW> jloc = json->get(u8"loc");
    std::shared_ptr<JsonW> jscripts = json->get(u8"scripts");

    if (jid == nullptr || jid->type() != JsonW::INTEGER)
    {
        LOG_E(tag_) << "init detect no/bad id in " << json->text();
        return false;
    }

    obj_id_ = (int)jid->integer();

    if (jtitle == nullptr || jtitle->type() != JsonW::INTEGER)
    {
        LOG_E(tag_) << "init detect no/bad title in " << json->text();
        return false;
    }

    title_ = table.find((int)jtitle->integer());

    if (title_ == nullptr)
    {
        LOG_E(tag_) << "init detect title " << jtitle->integer() << " does not exist in area's string table";
        return false;
    }

    if ( jloc == nullptr
        || jloc->type() != JsonW::ARRAY
        || jloc->size() != 3
        || jloc->get(0)->type() != JsonW::INTEGER
        || jloc->get(1)->type() != JsonW::INTEGER
        || jloc->get(2)->type() != JsonW::INTEGER)
    {
        LOG_E(tag_) << "init detect bad loc, only accept [int,int,int] format";
        return false;
    }

    loc_.set(
        (uint_fast32_t)jloc->get(0)->integer() + offset_x,
        (uint_fast32_t)jloc->get(1)->integer() + offset_y,
        (uint_fast32_t)jloc->get(2)->integer() + offset_z);

    // read the script
    if (jscripts != nullptr && jscripts->type() == JsonW::ARRAY && jscripts->size() > 0)
    {
        for (size_t i = 0; i < jscripts->size(); i++)
        {
            octillion::Script script;
            bool success = script.init(jscripts->get(i), markmap, offset_x, offset_y, offset_z);
            if (!success)
            {
                LOG_W(tag_) << "Interactive " << title_->str_.at(0) << " has corrupted script data";
            }
            else
            {
                scripts_.push_back(script);
            }
        }
    }

    return true;
}

std::shared_ptr<octillion::StringData> octillion::Interactive::title()
{
    return title_;
}

octillion::CubePosition octillion::Interactive::loc()
{
    return loc_;
}

int octillion::Interactive::area()
{
    return area_id_;
}

int octillion::Interactive::id()
{
    return obj_id_;
}
