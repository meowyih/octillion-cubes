#include "memory/memleak.hpp"

#ifdef MEMORY_DEBUG
void* operator new(size_t size)
{
    void* memory = MALLOC(size);

    MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

    std::cout << "new size: " << size << " addr:" << memory << std::endl;
    return memory;
}

void* operator new[](size_t size)
{
    void* memory = MALLOC(size);

    MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

    std::cout << "new[] size: " << size << " addr:" << memory << std::endl;
    return memory;
}

void operator delete(void* p)
{
    std::cout << "delete addr:" << p << std::endl;
    MemleakRecorder::instance().release(p);
    FREE(p);
}

void operator delete[](void* p)
{
    std::cout << "delete[] addr:" << p << std::endl;
    MemleakRecorder::instance().release(p);
    FREE(p);
}
#endif