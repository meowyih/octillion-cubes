#ifndef OCTILLION_MACRO_LOG_HEADER
#define OCTILLION_MACRO_LOG_HEADER

#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// usage:
// LOG_E() << "error message" << std::endl;
// LOG_W() << "warning message" << std::endl;
// LOG_I() << "information message" << std::endl;
// LOG_D() << "debug message" << std::endl;
//
// LOG_E("TAG") << "error message" << std::endl;
// LOG_W("TAG") << "warning message" << std::endl;
// LOG_I("TAG") << "information message" << std::endl;
// LOG_D("TAG") << "debug message" << std::endl;

// OCTILLION_MACRO_LOG_LEVEL 0 - disable all log
// OCTILLION_MACRO_LOG_LEVEL 1 - enable error
// OCTILLION_MACRO_LOG_LEVEL 2 - enable error, warning
// OCTILLION_MACRO_LOG_LEVEL 3 - enable error, warning, info
// OCTILLION_MACRO_LOG_LEVEL 4 - enable error, warning, info, debug
#ifndef OCTILLION_MACRO_LOG_LEVEL
#define OCTILLION_MACRO_LOG_LEVEL 3
#endif

#define LOG_E(x)  \
    if ( 1 > OCTILLION_MACRO_LOG_LEVEL ) ;\
    else octillion::MacroLog(x).log( 1 )

#define LOG_W(x) \
    if ( 2 > OCTILLION_MACRO_LOG_LEVEL ) ;\
    else octillion::MacroLog(x).log( 2 )
    
#define LOG_I(x) \
    if ( 3 > OCTILLION_MACRO_LOG_LEVEL ) ;\
    else octillion::MacroLog(x).log( 3 )
    
#define LOG_D(x) \
    if ( 4 > OCTILLION_MACRO_LOG_LEVEL ) ;\
    else octillion::MacroLog(x).log( 4 )
    
namespace octillion
{
    class MacroLog;
}

class octillion::MacroLog
{    
    public:    
        std::vector<std::string> blacklist_ = { "CoreServer", "RawProcessorClient", "RawProcessor" };
        
    public:        
        MacroLog( std::string tag = "" ) { tag_ = tag; }
        
        // 1. destructor will be called at the end of the if-else macro block
        // 2. C++11 defines ostream is atomic thread-safe (but data might interleaved)
        ~MacroLog() 
        {
            for( std::vector<std::string>::iterator it = blacklist_.begin(); it != blacklist_.end(); ++it ) 
            {
                if ( *it == tag_ )
                {
                    // skip it
                    return;
                }
            }
            
            os_ << std::endl;
            std::cout << os_.str();
            std::cout.flush();
        }
        

    public:
        std::ostringstream& log( int level )
        {
            std::string loglevel("");
            std::time_t curtime = std::time(0);
            std::tm now;
#ifdef WIN32
            localtime_s( &now, &curtime );
#else
            localtime_r(&curtime, &now);
#endif
            switch ( level )
            {
                case 1: loglevel = "e";  break;    
                case 2: loglevel = "w";  break;
                case 3: loglevel = "i";  break;
                case 4: loglevel = "d";  break;
                default: loglevel = "u";
            }
            
            
            os_ << "[" << loglevel << " " << now.tm_hour << ":" << now.tm_min << ":" << now.tm_sec << "]"; 
            
            if ( tag_.size() > 0 )
            {
                os_ << "[" << tag_ << "]";
            }
            
            os_ << " ";
            
            return os_;
        }
        
    private:
        std::string tag_;
        std::ostringstream os_;
};

#endif // OCTILLION_MACRO_LOG_HEADER