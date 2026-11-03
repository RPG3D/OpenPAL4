#pragma once

#include <cstdint>

namespace pal4 {

class IGobStateApplyRuntime {
public:
    virtual ~IGobStateApplyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeSnapshotTypeId(
        std::uint32_t snapshot) = 0;
};

// IDA 0x5a6960 is shared by Sound and Marker, and dispatches snapshot +4.
class GobStateApply final {
public:
    [[nodiscard]] static std::uint32_t InvokeTypeIfStored(
        std::uint32_t object, std::uint32_t snapshot,
        IGobStateApplyRuntime& runtime);
};

}  // namespace pal4
