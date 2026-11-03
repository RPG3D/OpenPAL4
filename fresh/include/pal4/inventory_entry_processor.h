#pragma once

#include <cstdint>

namespace pal4 {

struct InventoryEntryArrayState {
    std::uint32_t reserved{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity{};
};
static_assert(sizeof(InventoryEntryArrayState) == 16u);

class IInventoryEntryRuntime {
public:
    virtual ~IInventoryEntryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateStructMembers(
        std::uint32_t context, std::uint32_t entry) = 0;
};

class InventoryEntryRuntimeStub final : public IInventoryEntryRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t UpdateStructMembers(std::uint32_t context,
                                      std::uint32_t entry) override;
};

class IInventoryEntryInsertRuntime {
public:
    virtual ~IInventoryEntryInsertRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetGameDatabase() = 0;
    [[nodiscard]] virtual bool ContainsDatabaseKey(
        std::uint32_t array_address, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t LookupDatabaseKey(
        std::uint32_t array_address, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void CopyRange(std::uint32_t begin, std::uint32_t end,
                           std::uint32_t destination) = 0;
    virtual void FillOne(std::uint32_t destination,
                         std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class InventoryEntryInsertRuntimeStub final
    : public IInventoryEntryInsertRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t GetGameDatabase() override;
    bool ContainsDatabaseKey(std::uint32_t array_address,
                              std::uint32_t key) override;
    std::uint32_t LookupDatabaseKey(std::uint32_t array_address,
                                    std::uint32_t key) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void CopyRange(std::uint32_t begin, std::uint32_t end,
                   std::uint32_t destination) override;
    void FillOne(std::uint32_t destination,
                 std::uint32_t value) override;
    void Free(std::uint32_t address) override;
};

class InventoryEntryProcessor final {
public:
    [[nodiscard]] static std::uint32_t ProcessConditional(
        std::uint32_t context, std::uint32_t unused_argument,
        std::uint32_t entry, IInventoryEntryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessArray(
        InventoryEntryArrayState& state, std::uint32_t context,
        std::uint32_t unused_argument,
        IInventoryEntryRuntime& runtime);
    static void ProcessAndInsert(InventoryEntryArrayState& state,
                                 std::uint32_t inventory_entry,
                                 IInventoryEntryInsertRuntime& runtime);
};

}  // namespace pal4
