#pragma once

#include "pal4/string_buffer.h"

namespace pal4 {

// Native byte/storage adapter for the existing VC6 string algorithms.
// The supplied allocation owner must outlive every string allocated through it.
class NativeStringBufferRuntime final : public IStringAssignRuntime,
                                        public IStringDetachRuntime {
public:
    explicit NativeStringBufferRuntime(IStringBufferStorageRuntime& memory) : memory_(memory) {}
    void ThrowRange() override;
    void ThrowLength() override;
    void DetachAndReset(StringBufferState& state) override;
    bool AllocateText(StringBufferState& state,std::uint32_t length,std::uint8_t mode) override;
    void SetTextSize(StringBufferState& state,std::uint32_t length) override;
    void SetErrorFlag(StringBufferState& state,std::uint8_t flag) override;
    void Reallocate(StringBufferState& state,std::uint32_t length) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address,std::uint8_t value) override;
    void Copy(std::uint32_t destination,std::uint32_t source,std::uint32_t length) override;
    void Free(std::uint32_t address) override;
private:
    IStringBufferStorageRuntime& memory_;
};

} // namespace pal4
