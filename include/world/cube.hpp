#ifndef OCTILLION_CUBE_HEADER
#define OCTILLION_CUBE_HEADER

#include <cstdint>
#include <string>
#include <system_error>
#include <map>

#include "jsonw/jsonw.hpp"

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

namespace octillion
{
    class CubePosition;
    class Cube;
    class Area;
}

class octillion::CubePosition
{
private:
    const std::string tag_ = "CubePosition";

public:
    CubePosition();
    CubePosition(const CubePosition& rhs);
    CubePosition(uint32_t x, uint32_t y, uint32_t z);
    
    void set(uint32_t x, uint32_t y, uint32_t z);

    std::string str();
public:
    uint32_t x() { return x_axis_; }
    uint32_t y() { return y_axis_; }
    uint32_t z() { return z_axis_; }

    JsonObjectW* json();

    bool operator < (const CubePosition& rhs) const
    {
        if (x_axis_ < rhs.x_axis_)
        {
            return true;
        }
        else if (x_axis_ == rhs.x_axis_ && y_axis_ < rhs.y_axis_)
        {
            return true;
        }
        else if (x_axis_ == rhs.x_axis_ && y_axis_ == rhs.y_axis_ && z_axis_ < rhs.z_axis_)
        {
            return true;
        }

        return false;
    }

    CubePosition& operator = (const CubePosition& rhs)
    {
        x_axis_ = rhs.x_axis_;
        y_axis_ = rhs.y_axis_;
        z_axis_ = rhs.z_axis_;

        return *this;
    }

private:
    uint32_t x_axis_, y_axis_, z_axis_;

#ifdef MEMORY_DEBUG
public:
    static void* operator new(size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

    static void* operator new[](size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

        static void operator delete(void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }

    static void operator delete[](void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }
#endif
};

class octillion::Cube
{
private:
    const std::string tag_ = "Cube";

public:
    Cube(CubePosition loc);
    Cube(CubePosition loc, std::wstring wtitle, int areaid );
    Cube( const Cube& rhs );
    ~Cube();

public:
    CubePosition loc() { return loc_; }
    uint32_t area() { return areaid_; }
    bool addlink(Cube* dest);
    std::wstring wtitle() { return wtitle_; }

    // parameter type
    // 1 - Event::TYPE_JSON_SIMPLE, for login/logout/arrive/leave event usage
    // 2 - Event::TYPE_JSON_DETAIL, for detail event usage
    JsonObjectW* json(int type);
        
private:
    int areaid_ = 0;
    CubePosition loc_;
    std::wstring wtitle_;

public:
    uint8_t exits_[6];

#ifdef MEMORY_DEBUG
public:
    static void* operator new(size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

    static void* operator new[](size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

        static void operator delete(void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }

    static void operator delete[](void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }
#endif
};

class octillion::Area
{
private:
    const std::string tag_ = "Area";

public:
    Area(JsonTextW* json);
    ~Area();

    bool valid() { return valid_; }
    int id() { return id_; }
    std::wstring wtitle() { return wtitle_; }
    Cube* cube(CubePosition loc);

    int offset_x() { return offset_x_; }
    int offset_y() { return offset_y_; }
    int offset_z() { return offset_z_; }

public:
    std::map<CubePosition, Cube*> cubes_;

private:
    static bool readloc( JsonValueW* jvalue, CubePosition& pos, uint32_t offset_x, uint32_t offset_y, uint32_t offset_z );
    
private:
    bool valid_ = false;
    int id_;
    int offset_x_, offset_y_, offset_z_;
    std::wstring wtitle_;

#ifdef MEMORY_DEBUG
public:
    static void* operator new(size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

    static void* operator new[](size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

        static void operator delete(void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }

    static void operator delete[](void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }
#endif
};

#endif
