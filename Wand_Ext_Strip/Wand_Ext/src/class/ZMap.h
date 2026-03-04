#pragma once

#pragma pack(push, 1)

struct ZMapHeader
{
    uintptr_t vtbl;
    uintptr_t pBuckets;
    uint32_t  bucketCount;
};


// Generic ZMap node (key stored as uintptr_t)
struct ZMapNode
{
    uintptr_t unk0;   // +0
    uintptr_t pNext;  // +4
    uintptr_t key;    // +8 (uint32_t, pointer, etc.)
    // data starts at +12
};

// node: const char* -> ZXString<char>
struct ZMapNode_Str_ToZXString
{
    uintptr_t unk0;       // +0
    uintptr_t pNext;      // +4
    uintptr_t pKeyCStr;   // +8
};

#pragma pack(pop)
