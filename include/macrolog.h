#ifndef MACRO_LOG_HEADER
#define MACRO_LOG_HEADER

#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

// usage:
// LOG_E() << "error message" << std::endl;
// LOG_W() << "warning message" << std::endl;
// LOG_I() << "information message" << std::endl;
// LOG_D() << "debug message" << std::endl;

// MACRO_LOG_LEVEL 0 - disable all log
// MACRO_LOG_LEVEL 1 - enable error
// MACRO_LOG_LEVEL 2 - enable error, warning
// MACRO_LOG_LEVEL 3 - enable error, warning, info
// MACRO_LOG_LEVEL 4 - enable error, warning, info, debug
#ifndef MACRO_LOG_LEVEL
#define MACRO_LOG_LEVEL 4
#endif

#define LOG_E()  \
    if ( 1 > MACRO_LOG_LEVEL ) ;\
    else MacroLog().log( 1 )

#define LOG_W() \
    if ( 2 > MACRO_LOG_LEVEL ) ;\
    else MacroLog().log( 2 )
    
#define LOG_I() \
    if ( 3 > MACRO_LOG_LEVEL ) ;\
    else MacroLog().log( 3 )
    
#define LOG_D() \
    if ( 4 > MACRO_LOG_LEVEL ) ;\
    else MacroLog().log( 4 )       

class MacroLog
{
    public:
        MacroLog() {};
        
        // 1. destructor will be called at the end of the if-else macro block
        // 2. C++11 defines ostream is atomic thread-safe (but data might interleaved)
        ~MacroLog() 
        {
            os << std::endl;
            std::cout << os.str();
            std::cout.flush();
        }
        
    public:
        std::ostringstream& log( int level )
        {
            std::string loglevel("");
            std::time_t curtime = std::time(0);
            std::tm* now = std::localtime( &curtime );
            
            switch ( level )
            {
                case 1: loglevel = "e";  break;    
                case 2: loglevel = "w";  break;
                case 3: loglevel = "i";  break;
                case 4: loglevel = "d";  break;
                default: loglevel = "u";
            }
            
            
            os << "[" << loglevel << " " << now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec << "] "; 
            
            return os;
        }
        
    private:
        std::ostringstream os;
};

#endif // MACRO_LOG_HEADER