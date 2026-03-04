#pragma once
#include <cstdint>
// sizeof(TSecType<long>) = 0x0C
// [0] FakePtr1 (4 bytes)
// [4] FakePtr2 (4 bytes)
// [8] m_secdata -> TSecData<long>

#pragma pack(push, 1)
struct TSecType
{
    uint32_t fake1;
    uint32_t fake2;
    uintptr_t secDataPtr;     // pointer to RemoteTSecDataLong in game
};

// sizeof(TSecData<long>) = 0x0C
// [0] 4 encrypted bytes
// [4] bKey
// [5] FakePtr1
// [6] FakePtr2
// [8] checksum (2 bytes) + padding
struct TSecData
{
    uint32_t encData;   // 4 encrypted bytes
    uint8_t  bKey;
    uint8_t  fake1;
    uint8_t  fake2;
    uint16_t checksum;  // we won't actually use this
};

struct TSecDataDouble
{
    uint64_t data;       // 8 bytes - encrypted double (offset 0x00)
    uint8_t  bKey;       // 1 byte  (offset 0x08)
    uint8_t  FakePtr1;   // 1 byte  (offset 0x09)
    uint8_t  FakePtr2;   // 1 byte  (offset 0x0A)
    uint8_t  padding1;   // 1 byte  (offset 0x0B)
    uint16_t wChecksum;  // 2 bytes (offset 0x0C)
    uint8_t  padding2;   // 1 byte  (offset 0x0E)
    uint8_t  padding3;   // 1 byte  (offset 0x0F)
};


#pragma pack(pop)
