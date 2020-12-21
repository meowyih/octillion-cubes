#include <memory>

#include "world/stringtable.hpp"
#include "error/macrolog.hpp"

octillion::StringTable::StringTable()
{
}

bool octillion::StringTable::add(std::shared_ptr<JsonW> json)
{
    size_t size;
    std::shared_ptr<octillion::StringData> data = 
        std::make_shared< octillion::StringData>();

    if (json == nullptr)
        return false;

    if ( json->type() != JsonW::ARRAY )
        return false;

    size = json->size();

    for (size_t idx = 0; idx < size; idx++)
    {
        std::shared_ptr<JsonW> jstr = json->get(idx);
        std::shared_ptr<JsonW> jid, jtext, jattr;
        
        if (jstr == nullptr)
            return false;

        jid = jstr->get(u8"id");
        jtext = jstr->get(u8"text");
        jattr = jstr->get(u8"attr");

        if (jid == nullptr || jtext == nullptr)
            return false;

        if (jid->type() != JsonW::INTEGER || jtext->type() != JsonW::STRING)
            return false;

        data->id_ = (int)(jid->integer());
        data->str_ = jtext->str();
        data->wstr_ = jtext->wstr();

        auto it = data_.find(data->id_);
        if (it != data_.end())
        {
            LOG_E(tag_) << "duplicate string table, id:" << data->id_ << " str:" << data->str_ << "|" << (*it).second->str_;
            continue;
        }

        data_.insert(std::pair<int, std::shared_ptr<octillion::StringData>>(data->id_, data));

        // check attr
        if (jattr == nullptr || jattr->type() != JsonW::ARRAY)
            continue;

        for (size_t i = 0; i < jattr->size(); i++)
        {
            std::shared_ptr<JsonW> jint = jattr->get(i);
            if (jint == nullptr || jint->type() != JsonW::INTEGER)
                continue;
            data->attrs_.insert((int)(jint->integer()));
        }
    }

    return true;
}

std::shared_ptr<octillion::StringData> octillion::StringTable::find(int id)
{
    auto it = data_.find(id);
    if (it == data_.end())
    {
        return nullptr;
    }
    return it->second;
}
