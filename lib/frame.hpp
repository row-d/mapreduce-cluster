#pragma once
#include <cstdint>

namespace MapReducePICalculator
{
    constexpr uint16_t MAGIC_NUMBER = 0x4142;
    constexpr uint16_t PROTOCOL_PORT = 1337;

    enum InputFrameOpcode : uint8_t
    {
        REGISTER = 0x01,
        REGISTER_ACK,
        ASSIGN_MAP,
        MAP_RESULT,
        REDUCE_RESULT,
        HEARTBEAT,
        OPCODE_ERROR
    };

#pragma pack(push, 1)
    struct InputFrame
    {
        uint16_t magic;          // 2 bytes
        InputFrameOpcode opcode; // 1 bytes
        uint8_t length[3];       // 3 bytes
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

    inline uint64_t get_uint64(const uint8_t src[8])
    {
        return (static_cast<uint64_t>(src[0]) << 56) |
               (static_cast<uint64_t>(src[1]) << 48) |
               (static_cast<uint64_t>(src[2]) << 40) |
               (static_cast<uint64_t>(src[3]) << 32) |
               (static_cast<uint64_t>(src[4]) << 24) |
               (static_cast<uint64_t>(src[5]) << 16) |
               (static_cast<uint64_t>(src[6]) << 8) |
               (static_cast<uint64_t>(src[7]));
    }

    inline void set_uint64(uint8_t dest[8], uint64_t value)
    {
        dest[0] = (value >> 56) & 0xFF;
        dest[1] = (value >> 48) & 0xFF;
        dest[2] = (value >> 40) & 0xFF;
        dest[3] = (value >> 32) & 0xFF;
        dest[4] = (value >> 24) & 0xFF;
        dest[5] = (value >> 16) & 0xFF;
        dest[6] = (value >> 8) & 0xFF;
        dest[7] = value & 0xFF;
    }

}