#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

class IBaseObjectAtomicRuntime {
public:
    virtual ~IBaseObjectAtomicRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ForAllAtomics(
        std::uint32_t, const std::function<std::uint32_t(std::uint32_t)>&) {
        return 0u;
    }
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t) { return 0u; }
    [[nodiscard]] virtual std::int32_t UsedCount(std::uint32_t) { return 0; }
    [[nodiscard]] virtual std::uint32_t RecordAt(std::uint32_t,
                                                  std::int32_t) { return 0u; }
    [[nodiscard]] virtual std::string_view RecordText(std::uint32_t,
                                                       std::int32_t) {
        return {};
    }
    [[nodiscard]] virtual std::uint32_t StringLength(std::uint32_t) {
        return 0u;
    }
    virtual void InitString(BaseObjectState&, std::uint32_t,
                            std::uint32_t, std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t AppendString(
        BaseObjectState&, std::uint32_t, std::uint32_t,
        std::uint32_t) { return 0u; }
};

class BaseObjectAtomicProperties final {
public:
    // IDA 0x443ae0 and 0x443ba0.
    [[nodiscard]] static std::uint32_t ProcessFaceAtomics(
        BaseObjectState& state, IBaseObjectAtomicRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessLegAtomics(
        BaseObjectState& state, IBaseObjectAtomicRuntime& runtime);
    // IDA 0x443b10 and 0x443bd0; return zero to end enumeration on a match.
    [[nodiscard]] static std::uint32_t ProcessFaceAtomic(
        BaseObjectState& state, std::uint32_t atomic,
        IBaseObjectAtomicRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessLegAtomic(
        BaseObjectState& state, std::uint32_t atomic,
        IBaseObjectAtomicRuntime& runtime);
    // IDA 0x443c60 and 0x443cb0.
    [[nodiscard]] static std::uint32_t SetFaceProperty(
        BaseObjectState& state, std::uint32_t property,
        IBaseObjectAtomicRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetLegProperty(
        BaseObjectState& state, std::uint32_t property,
        IBaseObjectAtomicRuntime& runtime);
};

}  // namespace pal4
