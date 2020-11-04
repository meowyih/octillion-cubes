#include "world/worldmap.hpp"

int main()
{   
    octillion::WorldMap map;
    map.load_external_data_file( "../../data/" );    
    map.dump();
    
    return 0;
}