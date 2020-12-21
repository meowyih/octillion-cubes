#ifndef OCTILLION_CUBE_HEADER
#define OCTILLION_CUBE_HEADER

#include <cstdint>
#include <string>
#include <system_error>
#include <map>
#include <set>
#include <memory>

#include "jsonw/jsonw.hpp"
#include "world/stringtable.hpp"

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
    uint_fast32_t x() const { return x_axis_; }
    uint_fast32_t y() const { return y_axis_; }
    uint_fast32_t z() const { return z_axis_; }

    // convert cube position into json 
    std::shared_ptr<JsonW> json();

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

    bool operator == (const CubePosition& rhs) const
    {
        return x_axis_ == rhs.x_axis_ && y_axis_ == rhs.y_axis_ && z_axis_ == rhs.z_axis_;
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
};

// add a hash function for CubePosition for unordered_map usage
namespace std {

    template <>
    struct hash<octillion::CubePosition>
    {
        std::size_t operator()(const octillion::CubePosition& key) const
        {
            // we don't care about the overflow
            std::size_t a = (std::size_t)key.x();
            std::size_t b = (std::size_t)key.y();
            std::size_t c = (std::size_t)key.z();

            // cantor pairing function x 2
            std::size_t d = 5 * (a + b) * (a + b + 1) + b;
            return 5 * (c + d) * (c + d + 1) + d;
        }
    };
}

class octillion::Cube
{
private:
    const std::string tag_ = "Cube";

public:
    const static int X_INC = 1;
    const static int Y_INC = 2;
    const static int Z_INC = 4;
    const static int X_DEC = 3;
    const static int Y_DEC = 0;
    const static int Z_DEC = 5;

    const static int X_INC_Y_INC = 6;
    const static int X_INC_Y_DEC = 7;
    const static int X_DEC_Y_INC = 8;
    const static int X_DEC_Y_DEC = 9;
    const static int TOTAL_EXIT = 10;
	
	const static uint_fast32_t MOB_CUBE = 0x1;
	const static uint_fast32_t NPC_CUBE = 0x2;
	const static uint_fast32_t GOD_CUBE = 0x80000000;
    
    const static uint_fast32_t EXIT_NORMAL = 0x1;

	const static uint_fast32_t J_PLAYER = 0x01;
	const static uint_fast32_t J_MOB = 0x02;

public:
    Cube(const CubePosition& loc);
    Cube(const CubePosition& loc, std::shared_ptr<octillion::StringData> title, int areaid, uint_fast32_t attr );
    Cube( const Cube& rhs );
    ~Cube();

public:
    CubePosition loc() { return loc_; }
    uint_fast32_t area() { return areaid_; }
	bool addlink(std::shared_ptr<Cube> dest, uint_fast32_t attr);
    bool addlink(std::shared_ptr<Cube> dest);
    std::string title();
    std::wstring wtitle();

    // convert cube information into json
    // 1 - Event::TYPE_JSON_SIMPLE, for login/logout/arrive/leave event usage
    // 2 - Event::TYPE_JSON_DETAIL, for detail event usage
    std::shared_ptr<JsonW> json(int type);

	// help function, randomly pick one exit | exit_mask > 0.
	// and cube arribute | attr_mask > 0.
	// return -1 if no allowed enter
	int random_exit(uint8_t exit_mask)
	{
		std::vector<int> candidate;
		for (int idx = 0; idx < 6; idx++)
		{
			if ((exits_[idx] & exit_mask) > 0 )
			{
				candidate.push_back(idx);
			}
		}

		if (candidate.size() == 0)
		{
			return -1;
		}
		else
		{
			int idx = rand() % candidate.size();
			return candidate.at(idx);
		}
	}

public:
	static std::error_code json2attr(std::shared_ptr<JsonW> jattr, uint_fast32_t& attr);

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

	inline static int dir(std::shared_ptr<Cube> from, std::shared_ptr<Cube> to)
	{
		if (from->loc_.x() == to->loc_.x() && from->loc_.y() == to->loc_.y())
		{
			if (from->loc_.z() == to->loc_.z() + 1)
				return Z_INC;
			else if (from->loc_.z() == to->loc_.z() - 1)
				return Z_DEC;
		}
		else if (from->loc_.x() == to->loc_.x() && from->loc_.z() == to->loc_.z())
		{
			if (from->loc_.y() == to->loc_.y() + 1)
				return Y_INC;
			else if (from->loc_.y() == to->loc_.y() - 1)
				return Y_DEC;
		}
		else if (from->loc_.y() == to->loc_.y() && from->loc_.z() == to->loc_.z())
		{
			if (from->loc_.x() == to->loc_.x() + 1)
				return X_INC;
			else if (from->loc_.x() == to->loc_.x() - 1)
				return X_DEC;
		}

		return -1;
	}

	inline std::shared_ptr<Cube> find(std::map<CubePosition, std::shared_ptr<Cube>>& cubes_, int dir) 
	{
		CubePosition cbloc;
		switch (dir)
		{
		case X_INC: cbloc.set(loc_.x() + 1, loc_.y(), loc_.z()); break;
		case Y_INC: cbloc.set(loc_.x(), loc_.y() + 1, loc_.z()); break;
		case Z_INC: cbloc.set(loc_.x(), loc_.y(), loc_.z() + 1); break;
		case X_DEC: cbloc.set(loc_.x() - 1, loc_.y(), loc_.z()); break;
		case Y_DEC: cbloc.set(loc_.x(), loc_.y() - 1, loc_.z()); break;
        case Z_DEC: cbloc.set(loc_.x(), loc_.y(), loc_.z() - 1); break;
        case X_INC_Y_INC: cbloc.set(loc_.x() + 1, loc_.y() + 1, loc_.z()); break;
        case X_INC_Y_DEC: cbloc.set(loc_.x() + 1, loc_.y() - 1, loc_.z()); break;
        case X_DEC_Y_INC: cbloc.set(loc_.x() - 1, loc_.y() + 1, loc_.z()); break;
        case X_DEC_Y_DEC: cbloc.set(loc_.x() - 1, loc_.y() - 1, loc_.z()); break;
		default:
			return nullptr;
		}

		auto it = cubes_.find(cbloc);
		if (it == cubes_.end())
		{
			return nullptr;
		}
		return it->second;
	}

protected:
    int areaid_ = 0;

private:
	uint_fast32_t attr_ = 0xFFFFFFFF;
    CubePosition loc_;
    std::shared_ptr<octillion::StringData> title_ptr_;

public:
	uint_fast32_t exits_[TOTAL_EXIT];
    uint_fast32_t adjacent_cubes_[TOTAL_EXIT];
    uint_fast32_t adjacent_exits_[TOTAL_EXIT][TOTAL_EXIT];

	friend class WorldMap;

};

class octillion::Area
{
private:
    const std::string tag_ = "Area";

public:
    Area(std::shared_ptr<JsonW> json);
    ~Area();

    bool valid() { return valid_; }
    int id() { return id_; }
    std::string title() { return title_; }
    std::wstring wtitle() { return wtitle_; }
    std::shared_ptr<Cube> cube(CubePosition loc);

    int offset_x() { return offset_x_; }
    int offset_y() { return offset_y_; }
    int offset_z() { return offset_z_; }

public:
    // read the mark string and cube in "cubes" in json
    static bool getmark(
        const std::shared_ptr<JsonW> json, 
        std::map<std::string, std::shared_ptr<Cube>> &marks,
        const std::map<CubePosition, std::shared_ptr<Cube>>& cubes);

public:
    octillion::StringTable string_table_;
    std::map<CubePosition, std::shared_ptr<Cube>> cubes_;

public:
    static bool readloc( const std::shared_ptr<JsonW> jvalue, CubePosition& pos, uint_fast32_t offset_x, uint_fast32_t offset_y, uint_fast32_t offset_z );
	static bool addlink( bool is_2way, std::shared_ptr<Cube> from, std::shared_ptr<Cube> to, uint_fast32_t attr );
    static bool addlink( bool is_2way, std::shared_ptr<Cube> from, std::shared_ptr<Cube> to);

private:
    bool valid_ = false;
    int id_;
    uint_fast32_t offset_x_, offset_y_, offset_z_;
    std::string title_;
    std::wstring wtitle_;

private:

	friend class WorldMap;
};

#endif
