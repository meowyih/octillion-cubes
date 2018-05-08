
#include <system_error>
#include "ocerror.h"

// inject OcError into std::error
namespace 
{ 
    // anonymous namespace
    struct OcErrorCategory : std::error_category
    {
        const char* name() const noexcept override;
        std::string message(int ev) const override;
    };
      
    const char* OcErrorCategory::name() const noexcept
    {
        return "OcError";
    }
      
    std::string OcErrorCategory::message(int ev) const
    {
        switch (static_cast<OcError>(ev))
        {
            case OcError::E_SUCCESS:
                return "Success";
                
            case OcError::E_SERVER_BUSY:
                return "Server is already running";   
                
            case OcError::E_SYS_GETADDRINFO:
            case OcError::E_SYS_BIND:
            case OcError::E_SYS_FCNTL:
                return "call standard strerror( errno ) to get more information";
            
            case OcError::E_FATAL:
                return "Fatal error";
                
            default:
                return "Unknown error";
        }
    }
  
    const OcErrorCategory theOcErrorCategory {};
} // anonymous namespace
 
std::error_code make_error_code( OcError e )
{
    return {static_cast<int>(e), theOcErrorCategory};
}