#include <netinet/in.h>

#include "error/macrolog.hpp"
#include "error/ocerror.hpp"
#include "server/server.hpp"

#include "t005.hpp"

const std::string octillion::T005::tag_ = "T005";

octillion::T005::T005()
{
    LOG_D(tag_) << "T005()";
}

octillion::T005::~T005()
{
    LOG_D(tag_) << "~T005()";
}

void octillion::T005::test() 
{
    std::error_code err = octillion::Server::get_instance().start( "8888" );
}

void octillion::T005::stop() 
{
    std::error_code err = octillion::Server::get_instance().stop();
}