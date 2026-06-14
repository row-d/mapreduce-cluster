#include <cstdint>

namespace MapReduce
{
    enum FrameOpcode : uint8_t
    {
        REGISTER = 0x01,
        ASSIGN_MAP,
        MAP_RESULT,
        HEARTBEAT,
        ERROR
    };

#pragma pack(push, 1)
    struct Frame
    {
        uint16_t magic;    // 2 bytes
        FrameOpcode opcode;    // 1 bytes
        uint8_t length[3]; // 3 bytes
    };
#pragma pack(pop)

    inline void set_uint24(uint8_t dest[3], uint32_t value)
    {
        dest[0] = (value >> 16) & 0xFF;
        dest[1] = (value >> 8) & 0xFF;
        dest[2] = value & 0xFF;
    }

    inline uint32_t get_uint24(const uint8_t src[3])
    {
        return (static_cast<uint32_t>(src[0]) << 16) |
               (static_cast<uint32_t>(src[1]) << 8) |
               static_cast<uint32_t>(src[2]);
    }

}