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
    CubePosition(const CubePosition& rhs, int direction );
    CubePosition(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z);
    
    void set(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z);

    std::string str();
public:
    uint_fast32_t x() { return x_axis_; }
    uint_fast32_t y() { return y_axis_; }
    uint_fast32_t z() { return z_axis_; }

    // convert cube position into json 
    JsonW* json();

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
    uint_fast32_t x_axis_;
    uint_fast32_t y_axis_;
    uint_fast32_t z_axis_;

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
    const static int X_INC = 1;
    const static int Y_INC = 2;
    const static int Z_INC = 3;
    const static int X_DEC = 4;
    const static int Y_DEC = 5;
    const static int Z_DEC = 6;

public:
    Cube(const CubePosition& loc);
    Cube(const CubePosition& loc, const std::string& title, int areaid );
    Cube( const Cube& rhs );
    ~Cube();

public:
    CubePosition loc() { return loc_; }
    uint_fast32_t area() { return areaid_; }
    bool addlink(Cube* dest);
    std::string title() { return title_; }

    // convert cube information into json
    // 1 - Event::TYPE_JSON_SIMPLE, for login/logout/arrive/leave event usage
    // 2 - Event::TYPE_JSON_DETAIL, for detail event usage
    JsonW* json(int type);

public:
    inline static int opposite_dir(int dir)
    {
        switch (dir)
        {
        case X_INC: return X_DEC;
        case Y_INC: return Y_DEC;
        case Z_INC: return Z_DEC;
        case X_DEC: return X_INC;
        case Y_DEC: return Y_INC;
        case Z_DEC: return Z_INC;
        default:
            return 0;
        }
    }

    // static bool link(int type, Cube* from, Cube* to);
private:
    int areaid_ = 0;
    CubePosition loc_;
    std::string title_;

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
    Area(JsonW* json);
    ~Area();

    bool valid() { return valid_; }
    int id() { return id_; }
    std::string title() { return title_; }
    Cube* cube(CubePosition loc);

    int offset_x() { return offset_x_; }
    int offset_y() { return offset_y_; }
    int offset_z() { return offset_z_; }

public:
    // read the mark string and cube in "cubes" in json
    static bool getmark(
        const JsonW* json, 
        std::map<std::string, Cube*>& marks,
        const std::map<CubePosition, Cube*>& cubes);

public:
    std::map<CubePosition, Cube*> cubes_;

public:
    static bool readloc( JsonW* jvalue, CubePosition& pos, uint_fast32_t offset_x, uint_fast32_t offset_y, uint_fast32_t offset_z );
    static bool addlink( int linktype, Cube* from, Cube* to);
private:
    bool valid_ = false;
    int id_;
    uint_fast32_t offset_x_, offset_y_, offset_z_;
    std::string title_;

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
