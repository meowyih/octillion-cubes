#ifndef CACHE_HEADER
#define CACHE_HEADER

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "Point3d.hpp"

class CacheData1
{
public:
    CacheData1() {}
    CacheData1(int w, int h) : w_(w), h_(h) {}
    CacheData1(int w, int h, std::vector<Point3d>& in) : w_(w), h_(h), data_(in) {}

public:
    int w_ = 0;
    int h_ = 0;
    std::vector<Point3d> data_;
};

class Cache
{
private:
    const std::string tag_ = "Cache";

public:
    bool writeCache(int w_max, int h_max, int w, int h, wchar_t wchar, std::vector<Point3d>& in);
    bool readCache(int& w, int& h, wchar_t wchar, std::vector<Point3d>& out);

private:
    inline std::wstring create_key(int w, int h, wchar_t wchar);

private:
    std::unordered_map<std::wstring, std::shared_ptr<CacheData1>> cache_;

    // singleton
public:
    static Cache& get_instance()
    {
        static Cache instance;
        return instance;
    }

private:
    Cache();
    ~Cache();

public:
    // avoid accidentally copy
    Cache(Cache const&) = delete;
    void operator = (Cache const&) = delete;
};
#endif
