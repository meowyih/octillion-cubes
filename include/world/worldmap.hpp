#ifndef OCTILLION_WORLDMAP_HEADER
#define OCTILLION_WORLDMAP_HEADER

#include <string>
#include <vector>

#include "world/cube.hpp"

namespace octillion
{
    class WorldMap;
}

class octillion::WorldMap
{
private:
	const std::string global_version_ = "0.01";
	const std::string global_config_filename_ = "_global.json";

private:
    const std::string tag_ = "WorldMap";

public:
    WorldMap();
    ~WorldMap();
    
public:
    // read json and store map data into areas_ and cubes_
    bool load_external_data_file( std::string directory );
    
    // dump cubes_ and areas_ for debug purpose
    void dump();

private:
    // read a Cube pointer from global json
    static std::shared_ptr<octillion::Cube> readloc(
        std::shared_ptr<JsonW> json,
        std::map<std::string, std::shared_ptr<octillion::Cube>>& marks,
        std::map<octillion::CubePosition, std::shared_ptr<octillion::Cube>> cubes
    );

private:
    bool initialized_;
	std::string global_config_stamp_;
    
private:
    std::vector<std::shared_ptr<octillion::Area>> areas_;
    
    // shortcut to all cubes, it might cause cycle reference,
    // but it is not a problem since we will never have to release
    // the memory of cubes in the program lifecycle.
    std::map<CubePosition, std::shared_ptr<Cube>> cubes_;
    
    //octillion::CubePosition reborn_;
    std::shared_ptr<Cube> reborn_;
        
    friend class World;
};

#endif
