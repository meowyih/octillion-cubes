#ifndef OCTILLION_T005_HEADER
#define OCTILLION_T005_HEADER

#include <string>

namespace octillion
{
    class T005;    
}

class octillion::T005
{
    private:
        const static std::string tag_;

    public:
        T005();
        ~T005();	
        
	public:
        void test();
        void stop();
};

#endif // OCTILLION_T004_HEADER