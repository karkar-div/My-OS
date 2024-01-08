#include "memory.h"

uint64_t GetMemorySize(EFI_MEMORY_DESCRIPTOR* mMap, uint64_t mMapEntries, uint64_t mMapDescSize)
{
    uint64_t size = 0;
    for (int i = 0; i < mMapEntries; i++)
    {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + (i * mMapDescSize));
        size += desc->numPages * 4096;
    }

    return size;
}


void memset(void* start, uint8_t value, uint64_t num)
{
    for (uint64_t i = 0; i < num; i++)
        *(uint8_t*)((uint64_t)start + i) = value;
}

void memcpy(void* src, void* dest, uint64_t size)
{
    uint8_t* _src  = (uint8_t*)src;
    uint8_t* _dest = (uint8_t*)dest;
    while (size--)
    {
        *_dest = *_src;
        _dest++;
        _src++;
    }
}

int memcmp(void* src, void* dest, int amt)
{
    char* fi = (char*)src;
    char* la = (char*)dest;
    for (int i = 0; i < amt; i++)
    {
        if (fi[i] > la[i])
            return 1;
        if (fi[i] < la[i])
            return -1;
    }
    return 0;
}