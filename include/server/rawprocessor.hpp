#ifndef OCTILLION_RAW_PROCESSOR_HEADER
#define OCTILLION_RAW_PROCESSOR_HEADER

#include <map>
#include <cstdint>
#include <string>

#include "server/sslserver.hpp"

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

namespace octillion
{
    class RawProcessorClient;
    class RawProcessor;    
}

class octillion::RawProcessorClient
{
    private:
        const std::string tag_ = "RawProcessorClient";
        
    public:
        const static size_t kRawProcessorMaxKeyPoolSize = 9;
        const static size_t kRawProcessorHeaderSize = 4; // 4 bytes non-encrypted header
        
    public:
        RawProcessorClient();
        ~RawProcessorClient();
        
        int fd_;
        
        size_t headersize_;
        uint8_t header_[kRawProcessorHeaderSize]; 
        
        size_t keysize_;
        uint8_t key_[kRawProcessorMaxKeyPoolSize];
        
        size_t datasize_, dataanchor_;
        uint8_t* data_;

#ifdef MEMORY_DEBUG
public:
    static void* operator new(size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

    static void* operator new[](size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

        static void operator delete(void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }

    static void operator delete[](void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }
#endif
};

class octillion::RawProcessor : public octillion::SslServerCallback
{
    private:
        const static std::string tag_; // defined in rawprocessor.cpp
        
    public:
        RawProcessor();
        ~RawProcessor();
        
    public:
        // virtual function from SslServerCallback that handlers all incoming events
        virtual void connect( int fd ) override;
        virtual int recv( int fd, uint8_t* data, size_t datasize) override;
        virtual void disconnect( int fd ) override;
        
    public:
        static std::error_code senddata( int fd, uint8_t* data, size_t datasize );
        static std::error_code closefd(int fd);
        
    private:        
        static void encrypt( uint8_t* data, size_t datasize, uint8_t* key, size_t keysize );
        static void decrypt( uint8_t* data, size_t datasize, uint8_t* key, size_t keysize );

        int readheader( int fd, uint8_t* data, size_t datasize, size_t anchor );
        int readdata( int fd, uint8_t* data, size_t datasize, size_t anchor );

    private:
        std::map<int, RawProcessorClient> clients_;
        
    private:
        const static size_t kRawProcessorMaxDataChunkSize = 1024;

#ifdef MEMORY_DEBUG
public:
    static void* operator new(size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

    static void* operator new[](size_t size)
    {
        void* memory = MALLOC(size);
        
        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

    static void operator delete(void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }

    static void operator delete[](void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }
#endif
};

namespace octillion
{
    const uint8_t kRawProcessorKeyPool[] = {
      0x00, 0xe0, 0x07, 0xe7, 0x00, 0x7c, 0x07, 0xe7, 0xf8, 0xfe, 0x20, 0x0f,
      0xff, 0xff, 0xf8, 0x18, 0xff, 0x9f, 0xf8, 0x18, 0xff, 0x0f, 0xf8, 0x18,
      0xec, 0x1f, 0xc0, 0x78, 0x80, 0x7f, 0x00, 0x3f, 0x00, 0x7c, 0x00, 0x1f,
      0x40, 0xc0, 0x20, 0x0f, 0x37, 0xc0, 0x63, 0x0f, 0x3f, 0xc0, 0xe3, 0x87,
      0x7f, 0x83, 0xf7, 0xe0, 0x7f, 0x03, 0xff, 0xf0, 0x7f, 0x06, 0xbf, 0xf0,
      0xdb, 0xde, 0x9f, 0x61, 0xc0, 0xfe, 0x9e, 0x01, 0x80, 0xf3, 0xc2, 0x01,
      0x80, 0x27, 0x06, 0x03, 0x80, 0x0f, 0x05, 0xf2, 0x80, 0xfc, 0x31, 0xf8,
      0x03, 0xfc, 0x79, 0xd8, 0x03, 0xf8, 0xf1, 0xd9, 0x0f, 0xc0, 0xf1, 0xd1,
      0x3f, 0x07, 0x0e, 0x3f, 0xbc, 0x07, 0x0e, 0x2e, 0xfc, 0x07, 0x06, 0xae,
      0xcc, 0x23, 0x07, 0xfe, 0x6b, 0xf1, 0xc3, 0xfe, 0x3f, 0xfc, 0xdf, 0xb8,
      0x3f, 0xf4, 0x1f, 0x0d, 0x3f, 0xf4, 0x1f, 0x1c, 0x5f, 0xa1, 0xff, 0x41,
      0xc0, 0x09, 0xf0, 0x01, 0xc0, 0x0b, 0xe0, 0x01, 0xc0, 0x0e, 0xe0, 0x00,
      0xc0, 0xf1, 0xf8, 0x00, 0x81, 0xf1, 0xfc, 0x00, 0x01, 0xf0, 0x78, 0x78,
      0x01, 0xfc, 0x38, 0xfe, 0xc1, 0xf8, 0x34, 0xbf, 0xc1, 0xf8, 0x36, 0x1f,
      0xc6, 0x98, 0x36, 0x07, 0xfe, 0x07, 0x86, 0x07, 0x7e, 0x07, 0xc8, 0x5f,
      0x1f, 0x07, 0xc8, 0x7f, 0x1f, 0x07, 0x8d, 0xf8, 0x83, 0xff, 0xef, 0xf0,
      0x80, 0xfd, 0xe3, 0x80, 0xa0, 0x7c, 0xe3, 0x00, 0xf8, 0x3e, 0x0f, 0x05,
      0xf8, 0x3e, 0x0f, 0x17, 0x78, 0x74, 0x0e, 0x17, 0x07, 0xe0, 0x7c, 0xe7,
      0x0f, 0xc1, 0xf0, 0xef, 0x0f, 0x81, 0xf1, 0xea, 0x8f, 0x01, 0xe3, 0xe8,
      0xd2, 0x09, 0xe3, 0xf8, 0xd0, 0x08, 0x07, 0x7c, 0xd2, 0x0c, 0x1f, 0xfc,
      0x1e, 0x0c, 0x1f, 0xe1, 0x7c, 0x1e, 0x1f, 0xe1, 0xf8, 0x1f, 0xf0, 0xf1,
      0xf0, 0xf1, 0xf0, 0x50, 0x81, 0xf0, 0xfc, 0x10, 0x81, 0xf0, 0xd8, 0x00,
      0x03, 0xf2, 0x80, 0x07, 0x1f, 0x07, 0x00, 0x3b, 0x3f, 0x0c, 0x18, 0x39,
      0x1e, 0x0c, 0x18, 0x23, 0x1f, 0x0f, 0x9f, 0x87, 0x9f, 0x87, 0xd7, 0x86,
      0x87, 0x87, 0x87, 0x96, 0xc3, 0xc7, 0xc0, 0x03, 0xc3, 0xf0, 0xe0, 0x03,
      0xe1, 0xf0, 0x61, 0x41, 0xe0, 0xf0, 0x0f, 0xf9, 0xf4, 0x03, 0x0f, 0xb9,
      0x3c, 0x0f, 0x0f, 0xb7, 0x3c, 0x0f, 0x38, 0x27, 0x16, 0x7f, 0xf0, 0x06,
      0x0f, 0xfc, 0xf0, 0x44, 0x0f, 0x7d, 0xf0, 0x48, 0x3c, 0x3c, 0xf2, 0x48,
      0xf8, 0x3c, 0x0f, 0xe8, 0xf0, 0x38, 0x0f, 0xfc, 0xf0, 0x70, 0x0f, 0xfc,
      0xf0, 0xe0, 0x0b, 0xfc, 0xe1, 0xc0, 0x03, 0x7e, 0xe3, 0xc4, 0x90, 0x1f,
      0x87, 0xcf, 0xf0, 0x03, 0x87, 0xfd, 0xf4, 0x83, 0x00, 0xfc, 0x70, 0x83,
      0x00, 0x7c, 0x27, 0xff, 0x0c, 0x3e, 0x0f, 0xff, 0x0e, 0x37, 0xcf, 0x3f,
      0x0f, 0x03, 0xce, 0x19, 0xcb, 0xc3, 0xe8, 0x00, 0xe3, 0xc1, 0xc8, 0x00,
      0xc3, 0x81, 0x1e, 0x82, 0xdf, 0x8f, 0x1e, 0x82, 0x3e, 0x1e, 0x3d, 0xa6,
      0x30, 0x7c, 0x79, 0xed, 0xf0, 0x78, 0x79, 0xed, 0xe0, 0xf8, 0x78, 0xcd,
      0x07, 0x80, 0xe1, 0xdf, 0x0f, 0x87, 0xc3, 0xbe, 0x1f, 0x0f, 0xc6, 0x36,
      0x7c, 0x07, 0x86, 0x32, 0xf8, 0x07, 0x8e, 0x20, 0xe1, 0xc3, 0x8e, 0xf0,
      0xc1, 0xc3, 0x04, 0xff, 0xf0, 0x08, 0x00, 0xff, 0xf8, 0x3c, 0x00, 0x7f,
      0xf8, 0x3c, 0x00, 0x3f, 0xf8, 0x7c, 0x67, 0x01, 0xf0, 0x7d, 0xe7, 0x81,
      0xf9, 0xf7, 0xe7, 0x81, 0x3f, 0xd6, 0xf0, 0x03, 0x0f, 0x96, 0x70, 0x4b,
      0x0f, 0x1e, 0x72, 0x49, 0x0f, 0x9e, 0x72, 0x49, 0x07, 0xfc, 0xff, 0xf0,
      0xc1, 0xf1, 0xff, 0xb0, 0xc1, 0xf1, 0xed, 0xb4, 0xc3, 0xf1, 0xcc, 0x34,
      0x3f, 0x43, 0xcf, 0x70, 0x1f, 0x01, 0xcf, 0xe0, 0x0f, 0xc1, 0xcf, 0xe3,
      0x8f, 0x81, 0xcf, 0xff, 0xde, 0x01, 0xce, 0xff, 0xdc, 0x1a, 0x0e, 0xff,
      0xf8, 0x1e, 0x34, 0x3e, 0x3c, 0x1e, 0x31, 0x3a, 0x3f, 0xfc, 0x31, 0xe3,
      0x1f, 0xf0, 0xe1, 0xe3, 0x1f, 0xf0, 0xe3, 0xc5, 0xf0, 0x0f, 0xdf, 0x04,
      0xe0, 0x0f, 0x9e, 0x04, 0xe0, 0x1f, 0x78, 0x04, 0xe0, 0x3e, 0x78, 0x04,
      0x20, 0xf8, 0x78, 0x04, 0x3c, 0x3c, 0x78, 0x05, 0x7c, 0x0e, 0xff, 0xff,
      0xf8, 0x04, 0x3f, 0xff, 0xf1, 0xe0, 0x0f, 0xff, 0xbf, 0xf0, 0x00, 0x17,
      0x3f, 0xf0, 0x00, 0x07, 0x3f, 0xfb, 0x00, 0x00, 0x3c, 0x7f, 0xc0, 0x00,
      0xc3, 0xe3, 0xd3, 0x68, 0xc3, 0xc1, 0xd7, 0xf8, 0xc3, 0xc1, 0xd7, 0xf8,
      0xc3, 0x83, 0xff, 0xf8, 0x80, 0x03, 0xfc, 0x78, 0x70, 0x07, 0xfc, 0x78,
      0x78, 0x3f, 0x3c, 0x07, 0xf8, 0x7c, 0x00, 0x07, 0xbf, 0xfc, 0x01, 0x07,
      0x07, 0xf8, 0x01, 0xf7, 0x07, 0xd8, 0x00, 0x7f, 0x27, 0xfe, 0x02, 0x18,
      0xe0, 0xfc, 0x42, 0x08, 0xf0, 0xfc, 0xe2, 0x08, 0x70, 0x7c, 0xe3, 0x00,
      0x3f, 0x00, 0xff, 0xe0, 0x3f, 0x00, 0xf3, 0xe1, 0xff, 0x03, 0xf0, 0xe5,
      0xff, 0x03, 0xd0, 0xe5, 0xf0, 0x0f, 0xc0, 0xc3, 0xf0, 0x83, 0xfe, 0x41,
      0x65, 0x81, 0xfe, 0x01, 0x07, 0x83, 0xfe, 0xb8, 0x03, 0x87, 0xff, 0xfe,
      0x03, 0x07, 0xff, 0xfe, 0x07, 0x07, 0xff, 0xfe, 0x0f, 0x07, 0xff, 0xfe,
      0x3e, 0x0f, 0xcf, 0xec, 0x3c, 0x3e, 0x07, 0xcd, 0x3c, 0x3e, 0x00, 0x01,
      0xa7, 0x1e, 0x3c, 0x13, 0xc3, 0xc0, 0x3c, 0x7b, 0xc0, 0xe0, 0x3e, 0xf8,
      0xf0, 0xf0, 0x1f, 0xe0, 0xf8, 0x7c, 0x00, 0x00, 0xf8, 0xf8, 0x00, 0x00,
      0xf1, 0xf8, 0x40, 0x07, 0xf3, 0xe0, 0x03, 0xff, 0xe7, 0xc0, 0x13, 0xff,
      0xc7, 0x80, 0x1f, 0xff, 0xcf, 0x0a, 0x6c, 0x01, 0xcf, 0x0f, 0xec, 0x00,
      0x8f, 0x03, 0xec, 0x00, 0x0f, 0x03, 0xe0, 0x00, 0x07, 0xcb, 0xe0, 0x80,
      0x17, 0xfc, 0x49, 0x80, 0xf8, 0x30, 0x47, 0x3c, 0xf8, 0x15, 0xfe, 0x3c,
      0xd0, 0x01, 0xfc, 0x39, 0x82, 0x03, 0x7e, 0x09, 0x83, 0xe1, 0xfe, 0x81,
      0x01, 0xf0, 0xf9, 0xd3, 0x01, 0xf0, 0xf9, 0xdf, 0x01, 0xf8, 0x79, 0xdf,
      0x01, 0xf8, 0x78, 0xfc, 0x21, 0xfc, 0x18, 0xf8, 0xfe, 0x0f, 0x08, 0x6c,
      0xfe, 0x0f, 0x01, 0x2c, 0xfe, 0x0f, 0x83, 0x0f, 0xa4, 0x1f, 0x07, 0x0f,
      0x85, 0xf0, 0x7e, 0x37, 0x07, 0xe1, 0xfe, 0xf0, 0x0f, 0xe3, 0xfe, 0xf0,
      0x1e, 0x5f, 0xc7, 0xf0, 0x78, 0x1f, 0x0f, 0xf8, 0xf8, 0x38, 0x3f, 0x8f,
      0xf8, 0x00, 0x7f, 0x8f, 0x07, 0x80, 0x7d, 0x87, 0x0f, 0x83, 0x87, 0x07,
      0x1f, 0x87, 0x80, 0x05, 0x3f, 0x8f, 0x80, 0x60, 0x3e, 0x3f, 0x40, 0x70,
      0x7c, 0x7c, 0xe0, 0x78, 0x58, 0x79, 0xc0, 0xfa, 0xc3, 0xe3, 0x97, 0xfa,
      0xc3, 0xe7, 0xbf, 0xc8, 0xc1, 0xe1, 0xff, 0xd8, 0xe1, 0xf0, 0xff, 0xd8,
      0xe1, 0xf0, 0xff, 0xf0, 0x07, 0xf0, 0x7f, 0xe0, 0x07, 0xcf, 0x83, 0xe7,
      0x0f, 0x0f, 0x83, 0x27, 0x1f, 0x0f, 0x83, 0xff, 0xdf, 0x07, 0x81, 0xff,
      0xe8, 0x70, 0x60, 0x7f, 0xfc, 0x3c, 0x38, 0x1c, 0x7c, 0x3e, 0x18, 0x00,
      0xf8, 0x1f, 0x9e, 0x01, 0xf8, 0x07, 0xbf, 0xc1, 0x07, 0x07, 0x7b, 0xe1,
      0x07, 0x87, 0x03, 0xe0, 0xc3, 0xe8, 0x0e, 0x00, 0xc3, 0xf8, 0x1f, 0x00,
      0x40, 0x7c, 0x1f, 0x80, 0xf8, 0x3c, 0x70, 0x33, 0xde, 0x30, 0xf0, 0x73,
      0x0f, 0x3c, 0xf0, 0xf3, 0x0f, 0x3c, 0x3c, 0xff, 0x1f, 0x1c, 0x3f, 0xfe,
      0x3e, 0x1e, 0x7f, 0x0c, 0x3e, 0x53, 0x6f, 0x0d, 0xbc, 0xd3, 0x0f, 0x0d,
      0xb6, 0xc3, 0x2f, 0x6d, 0x81, 0x83, 0xef, 0x70, 0xc1, 0x87, 0xff, 0x00,
      0xc1, 0x07, 0xf4, 0x00, 0xc8, 0x07, 0xf5, 0x8f, 0x3e, 0x1c, 0x31, 0xfb,
      0x3c, 0x78, 0x33, 0xf8, 0x78, 0x78, 0x03, 0xf8, 0xf0, 0xf0, 0x00, 0x24,
      0xf0, 0xe7, 0xc0, 0x06, 0x68, 0x1f, 0x88, 0x06, 0x0e, 0x1e, 0x1c, 0x06,
      0x0e, 0x1c, 0x30, 0x07, 0xc0, 0x79, 0x00, 0x1f, 0xe0, 0xe1, 0x80, 0x3f,
      0xe0, 0xe0, 0x07, 0xfd, 0xf0, 0xf0, 0x3f, 0xf8, 0xfe, 0x90, 0x1f, 0xf0,
      0x7f, 0x84, 0x07, 0xc0, 0x3f, 0x8e, 0x02, 0x71, 0x1f, 0x0f, 0x02, 0x79,
      0x1f, 0x1f, 0xc0, 0x7f, 0x07, 0x0f, 0xe0, 0x0f, 0x83, 0x8e, 0x40, 0x07,
      0xe1, 0xe0, 0x08, 0xe7, 0xf1, 0xe0, 0x0a, 0xff, 0xf0, 0xf0, 0x02, 0xfc,
      0xf8, 0x61, 0xa6, 0xfc, 0xf8, 0x63, 0xf6, 0xd8, 0xfa, 0x47, 0xfd, 0x00,
      0x86, 0x1f, 0xfd, 0x06, 0x86, 0x3e, 0xf9, 0x07, 0x00, 0x76, 0xff, 0x03,
      0x30, 0x60, 0x7f, 0xf9, 0xa1, 0xc0, 0x7f, 0xf8, 0x07, 0xc8, 0xd1, 0x4b,
      0x0f, 0x89, 0x80, 0x07, 0x0f, 0xc3, 0xc2, 0x0c, 0x07, 0xc3, 0xc3, 0xf8,
      0x07, 0xe3, 0xc3, 0xfb, 0x3c, 0x38, 0x3f, 0x03, 0x3c, 0x38, 0x3f, 0x03,
      0x7c, 0xf8, 0x3f, 0x83, 0x6d, 0xf4, 0x0f, 0xc1, 0x1f, 0x1f, 0x87, 0xf8,
      0x1e, 0x0e, 0xf9, 0xfc, 0x3c, 0x1e, 0xf9, 0xfc, 0x7c, 0x1c, 0xd9, 0xfc,
      0xfc, 0x79, 0x40, 0x2c, 0xf0, 0xe1, 0x20, 0x0f, 0xe0, 0xe3, 0x20, 0x0f
    };
    const unsigned int kRawProcessorKeyPoolSize = 1212;
}


#endif // OCTILLION_RAW_PROCESSOR_HEADER