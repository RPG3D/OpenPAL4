#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ConditionValueArrayState {
    std::array<std::uint32_t, 45> unknown{};
    std::uint32_t begin{};  // +0xb4
    std::uint32_t end{};    // +0xb8
    std::uint32_t capacity_end{};  // +0xbc
};
static_assert(sizeof(ConditionValueArrayState) == 0xc0u);
static_assert(offsetof(ConditionValueArrayState, begin) == 0xb4u);
static_assert(offsetof(ConditionValueArrayState, end) == 0xb8u);
static_assert(offsetof(ConditionValueArrayState, capacity_end) == 0xbcu);

class IConditionValueArrayRuntime {
public:
    virtual ~IConditionValueArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ConditionValueArrayRuntimeStub final : public IConditionValueArrayRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
};

struct ConditionValueIteratorImage {
    std::uint32_t vtable{};
    std::uint32_t current{};
    std::uint32_t owner{};
};
static_assert(sizeof(ConditionValueIteratorImage) == 12u);

class IConditionValueIteratorRuntime {
public:
    virtual ~IConditionValueIteratorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Store(std::uint32_t address,
                       const ConditionValueIteratorImage& image) = 0;
    virtual void WriteOutput(std::uint32_t address, std::uint32_t value) = 0;
};

class ConditionValueIteratorRuntimeStub final
    : public IConditionValueIteratorRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Store(std::uint32_t address,
               const ConditionValueIteratorImage& image) override;
    void WriteOutput(std::uint32_t address, std::uint32_t value) override;
};

class ConditionValueArray final {
public:
    static constexpr std::uint32_t kIteratorVtable = 0x842bfcu;
    [[nodiscard]] static bool Contains(const ConditionValueArrayState& state,
                                       std::uint32_t value,
                                       IConditionValueArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPresent(
        ConditionValueArrayState& state, std::uint32_t value,
        std::uint32_t enabled, IConditionValueArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetIterator(
        const ConditionValueArrayState& state, std::uint32_t object_address,
        std::uint32_t output_address, IConditionValueIteratorRuntime& runtime);
};

}  // namespace pal4
