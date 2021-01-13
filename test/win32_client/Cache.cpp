#include "error/macrolog.hpp"

#include "Cache.hpp"

bool Cache::writeCache(int w_max, int h_max, int w, int h, wchar_t wchar, std::vector<Point3d>& in)
{
    std::shared_ptr<CacheData1> data;
    std::wstring key = create_key(w_max, h_max, wchar);

    // remove old cache
    auto it = cache_.find(key);
    if (it != cache_.end())
    {
        cache_.erase(it);
    }

    data = std::make_shared<CacheData1>(w, h, in);
    cache_.insert({ key, data });

    if (cache_.find(key) == cache_.end())
    {
        LOG_E(tag_) << "write failed";
        return false;
    }

    return true;
}

bool Cache::readCache(int& w, int& h, wchar_t wchar, std::vector<Point3d>& out)
{
    std::wstring key = create_key(w, h, wchar);

    auto it = cache_.find(key);

    if (it == cache_.end())
    {
        LOG_D(tag_) << "read cache, but does not exist";
        return false;
    }

    out.assign(it->second->data_.begin(), it->second->data_.end());

    return true;
}

inline std::wstring Cache::create_key(int w, int h, wchar_t wchar)
{
    std::wstring key;
    key = key + wchar + std::to_wstring(w) + std::to_wstring(h);
    return key;
}

Cache::Cache()
{
}

Cache::~Cache()
{
}
