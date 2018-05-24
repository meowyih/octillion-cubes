#ifndef OCTILLION_WORLD_HEADER
#define OCTILLION_WORLD_HEADER

#include <map>
#include <mutex>

#include "error/macrolog.hpp"
#include "error/ocerror.hpp"

#include "world/tick.hpp"
#include "world/cube.hpp"
#include "world/player.hpp"
#include "world/command.hpp"

namespace octillion
{
    class World;
}

class octillion::World : octillion::Tick, octillion::TickCallback
{
private:
    const std::string tag_ = "World";
    
public:
    //Singleton
    static World& get_instance()
    {
        static World instance;
        return instance;
    }

    // avoid accidentally copy
    World(World const&) = delete;
    void operator = (World const&) = delete;

private:
    World();
    ~World();

public:
    std::error_code login(int pcid);
    std::error_code logout(int pcid);
    std::error_code move(int pcid, const CubePosition& loc );
    std::error_code move( int pcid, CubePosition::Direction dir );

public:
    
    virtual void tick() override;
    virtual void tickcallback(
        uint32_t type,
        uint32_t param1,
        uint32_t param2) override;
private:
    void addcmd(Command* cmd);
    std::mutex cmds_lock_;
    std::map<uint32_t, Command*> cmds_;

private:
    std::map<CubePosition, Cube*> cubes_;
    std::map<uint32_t, Player*> pcs_;

};

#endif
