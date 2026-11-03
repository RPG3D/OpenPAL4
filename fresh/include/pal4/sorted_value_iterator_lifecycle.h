#pragma once

#include <cstdint>

namespace pal4 {

class ISortedValueIteratorLifecycleRuntime {
public:
    virtual ~ISortedValueIteratorLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class SortedValueIteratorLifecycleRuntimeStub final
    : public ISortedValueIteratorLifecycleRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    void Free(std::uint32_t address) override;
};

class SortedValueIteratorLifecycle final {
public:
    static constexpr std::uint32_t kValidVtable = 0x842bfcu;
    static constexpr std::uint32_t kInvalidVtable = 0x842c28u;
    static constexpr std::uint32_t kBaseVtable = 0x842c54u;

    static void SetBaseVtable(
        std::uint32_t address, ISortedValueIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneValid(
        std::uint32_t source, ISortedValueIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneInvalid(
        std::uint32_t source, ISortedValueIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t FieldAddress(std::uint32_t address);
    [[nodiscard]] static std::uint32_t DeleteBase(
        std::uint32_t address, std::uint32_t flags,
        ISortedValueIteratorLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteDerived(
        std::uint32_t address, std::uint32_t flags,
        ISortedValueIteratorLifecycleRuntime& runtime);
};

}  // namespace pal4
