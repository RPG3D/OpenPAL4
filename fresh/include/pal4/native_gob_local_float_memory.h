#pragma once

#include "pal4/gob_local_float.h"
#include "pal4/native_base_object_memory.h"

#include <bit>

namespace pal4 {

// Borrowed native memory endpoints shared by LocalFloat's phase adapters.
class NativeGobLocalFloatMemory : public virtual IGobLocalFloatMemoryRuntime {
public:
    explicit NativeGobLocalFloatMemory(NativeBaseObjectMemory& memory) : float_memory_(memory) {}
    std::uint32_t Allocate(std::uint32_t size) override { return float_memory_.Allocate(size); }
    void Free(std::uint32_t address) override { (void)float_memory_.Free(address); }
    std::uint32_t FreePathBuffer(std::uint32_t address) override { return float_memory_.Free(address); }
    std::uint32_t ReadWord(std::uint32_t address) override { return float_memory_.ReadDword(address); }
    void WriteWord(std::uint32_t address, std::uint32_t value) override { float_memory_.WriteDword(address, value); }
    float ReadFloat(std::uint32_t address) override { return std::bit_cast<float>(ReadWord(address)); }
    void WriteFloat(std::uint32_t address, float value) override { WriteWord(address, std::bit_cast<std::uint32_t>(value)); }

private:
    NativeBaseObjectMemory& float_memory_;
};

}  // namespace pal4
