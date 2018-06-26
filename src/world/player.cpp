
#include <ostream>
#include <sstream>
#include <string>
#include <cstdlib>

#include "world/cube.hpp"
#include "world/player.hpp"

void octillion::Player::move(CubePosition loc)
{
    loc_ = loc;
}

void octillion::Player::roll(uint32_t gender, uint32_t cls, std::map<std::string, uint32_t>& attribute)
{
    // con / luc / men / cha = 1 - 10
    // con + luc = 10 - 12
    // men + cha = 10 - 12
    uint32_t sumConLuc = rand() % 3 + 10;
    uint32_t sumMenCha = rand() % 3 + 10;
    uint32_t con = rand() % 9 + 1;
    uint32_t luc = sumConLuc - con;
    uint32_t men = rand() % 9 + 1;
    uint32_t cha = sumMenCha - men;

    attribute["con"] = con;
    attribute["luc"] = luc;
    attribute["men"] = men;
    attribute["cha"] = cha;

    return;
}

std::string octillion::Player::rollname()
{
    std::ostringstream stringStream;
    stringStream << "wizard123";
    return stringStream.str();
}