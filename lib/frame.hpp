#include <cstdint>

namespace MapReducePICalculator
{
    constexpr uint16_t MAGIC_NUMBER = 0x4142;

    enum InputFrameOpcode : uint8_t
    {
        REGISTER = 0x01,
        ASSIGN_MAP,
        MAP_RESULT,
        HEARTBEAT,
        ERROR
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
        return (static_cast<uint64_t>(src[0]) << 64) |
               (static_cast<uint64_t>(src[1]) << 56) |
               (static_cast<uint64_t>(src[2]) << 48) |
               (static_cast<uint64_t>(src[3]) << 40) |
               (static_cast<uint64_t>(src[4]) << 32) |
               (static_cast<uint64_t>(src[5]) << 24) |
               (static_cast<uint64_t>(src[6]) << 16) |
               (static_cast<uint64_t>(src[7]) << 8);
    }

    inline uint64_t set_uint64(uint8_t dest[8], uint64_t value)
    {
        dest[0] = (value >> 64) & 0xFF;
        dest[1] = (value >> 56) & 0xFF;
        dest[2] = (value >> 48) & 0xFF;
        dest[3] = (value >> 40) & 0xFF;
        dest[4] = (value >> 32) & 0xFF;
        dest[5] = (value >> 24) & 0xFF;
        dest[6] = (value >> 16) & 0xFF;
        dest[7] = value & 0xFF;
    }

    template <typename T>
    inline void get_type(const T *src, size_t bytes)
    {
        T result;
        size_t it = bytes;
        for (size_t i = 0; i < bytes && it > -1; i++)
        {
            result |= (static_cast<uint64_t>(*(dest + i)) << it);
            it = it - 8;
        }
        return result;
    }

    template <typename T, typename R>
    inline R set_type(T *dest, size_t bytes, R value)
    {
        size_t it = bytes;
        for (size_t i = 0; i < bytes && it > -1; i++)
        {
            *(dest + i) = (value >> it) & 0xFF;
            it = it - 8;
        }
    }

}