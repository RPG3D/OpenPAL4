#pragma once

#include <cstdint>

namespace pal4 {

class IUiObjectDeletingRuntime {
public:
    virtual ~IUiObjectDeletingRuntime() = default;
    virtual void CallBaseRoutine(std::uint32_t object) = 0;
    virtual void Free(std::uint32_t object) = 0;
};

class UiObjectDeletingDestructor final {
public:
    // IDA UiObjectDeletingDestructor_Delete at 0x549e10.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        IUiObjectDeletingRuntime& runtime);
};

}  // namespace pal4
