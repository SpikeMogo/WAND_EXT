#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct ZRef
{
    int       refCount;   
    uintptr_t p;          // pointer to T (remote)
};

struct ZArray
{
    uintptr_t a;          // pointer to first ZRefRemote<T>
};
#pragma pack(pop)
