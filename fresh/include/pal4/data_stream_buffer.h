#pragma once

#include <cstdint>

namespace pal4 {

class IDataStreamBufferRuntime {
public:
    virtual ~IDataStreamBufferRuntime() = default;
    virtual void Free(std::uint8_t* buffer) = 0;
    virtual void Read(void* stream, void* buffer, std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint8_t* Allocate(std::uint32_t size) = 0;
};

class DataStreamBuffer final {
public:
    static void ReadInto(void* stream, std::uint8_t*& output,
                         IDataStreamBufferRuntime& runtime);
};

}  // namespace pal4
