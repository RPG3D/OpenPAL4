#pragma once
#include "pal4/data_stream_buffer.h"

namespace pal4 {
// Stream handles belong to the started native SDK. Allocation stays with the
// game allocator; callers must supply a matching allocate/free pair.
class NativeDataStreamBufferRuntime final : public IDataStreamBufferRuntime {
public:
    using AllocateBuffer = std::uint8_t* (*)(std::uint32_t);
    using FreeBuffer = void (*)(std::uint8_t*);
    NativeDataStreamBufferRuntime(AllocateBuffer allocate, FreeBuffer free);
    void Free(std::uint8_t* buffer) override;
    void Read(void* stream, void* buffer, std::uint32_t size) override;
    std::uint8_t* Allocate(std::uint32_t size) override;
private:
    AllocateBuffer allocate_;
    FreeBuffer free_;
};
}
