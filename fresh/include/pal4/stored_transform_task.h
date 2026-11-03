#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IStoredTransformTaskRuntime {
public:
    virtual ~IStoredTransformTaskRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestruct(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ApplyStoredTransform(
        std::uint32_t owner,
        const std::array<std::uint32_t, 3>& zero_vector) = 0;
    [[nodiscard]] virtual bool WaitComplete(std::uint32_t object) = 0;
};

class StoredTransformTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8488d0u;
    // IDA 0x598170..0x598210.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IStoredTransformTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IStoredTransformTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Apply(
        std::uint32_t object, IStoredTransformTaskRuntime& runtime);
    [[nodiscard]] static bool IsWaitComplete(
        std::uint32_t object, IStoredTransformTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetOwner(
        std::uint32_t object, std::uint32_t owner,
        IStoredTransformTaskRuntime& runtime);
};

}  // namespace pal4
