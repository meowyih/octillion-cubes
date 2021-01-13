#include <memory>

#include "world/stringtable.hpp"
#include "error/macrolog.hpp"

octillion::StringTable::StringTable()
{
}

bool octillion::StringTable::add(std::shared_ptr<JsonW> json)
{
    size_t size;

    if (json == nullptr)
        return false;

    if ( json->type() != JsonW::ARRAY )
        return false;

    size = json->size();

    for (size_t idx = 0; idx < size; idx++)
    {
        std::shared_ptr<octillion::StringData> data =
            std::make_shared< octillion::StringData>();

        std::shared_ptr<JsonW> jstr = json->get(idx);
        std::shared_ptr<JsonW> jid, jtext, jselect;
        
        if (jstr == nullptr)
            return false;

        jid = jstr->get(u8"id");
        jtext = jstr->get(u8"text");
        jselect = jstr->get(u8"select");

        if (jid == nullptr || jtext == nullptr)
            return false;

        if (jid->type() != JsonW::INTEGER)
            return false;

        data->id_ = (int)(jid->integer());

        if (jtext->type() == JsonW::STRING)
        {
            data->str_.push_back( jtext->str() );
            data->wstr_.push_back( jtext->wstr() );
        }
        else if (jtext->type() == JsonW::ARRAY)
        {
            for (size_t i = 0; i < jtext->size(); i++)
            {
                std::shared_ptr<JsonW> jtxt = jtext->get(i);
                if (jtxt->type() != JsonW::STRING)
                    return false;
                data->str_.push_back(jtxt->str());
                data->wstr_.push_back(jtxt->wstr());
            }
        }
        
        auto it = data_.find(data->id_);
        if (it != data_.end())
        {
            LOG_E(tag_) << "duplicate string table, id:" << data->id_ << " " << data->str_.at(0);
            continue;
        }

        // check select type
        if (jselect != nullptr && jselect->type() == JsonW::STRING)
        {
            if (jselect->str().compare("random") == 0)
            {
                data->select_ = octillion::StringTable::SELECT_RANDOMLY;
            }
            else if (jselect->str().compare("order") == 0)
            {
                data->select_ = octillion::StringTable::SELECT_ORDERLY;
            }
            else
            {
                LOG_W(tag_) << "bad string select type, string table, id:" << data->id_ << " " << data->str_.at(0) << " select:" << jselect->str();
            }
        }

        // insert data
        data_.insert(std::pair<int, std::shared_ptr<octillion::StringData>>(data->id_, data));
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
