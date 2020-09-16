#ifndef OCTILLION_T004_HEADER
#define OCTILLION_T004_HEADER

#include <string>

namespace octillion
{
    class T004;    
}

class octillion::T004
{
    private:
        const static std::string tag_;

    public:
        T004();
        ~T004();	
        
	public:
        void test();
        bool valid( uint8_t* data, size_t datasize );
        
    private:
        size_t datasize_;
        uint8_t* data_;
};

#endif // OCTILLION_T004_HEADER