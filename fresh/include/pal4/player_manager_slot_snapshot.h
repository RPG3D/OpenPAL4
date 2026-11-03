#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct PlayerManagerScriptSnapshot {
    static constexpr std::uint32_t kWordCount = 0x2ccu / 4u;
    static constexpr std::uint32_t kRecordWords = 0x150u / 4u;
    static constexpr std::uint32_t kFirstRecord = 1u;
    static constexpr std::uint32_t kSecondRecord = 85u;
    static constexpr std::uint32_t kTripleOwner = 169u;
    static constexpr std::uint32_t kPairOwner = 173u;
    std::array<std::uint32_t, kWordCount> words{};
};
static_assert(sizeof(PlayerManagerScriptSnapshot) == 0x2ccu);

class IPlayerManagerSlotSnapshotRuntime {
public:
    virtual ~IPlayerManagerSlotSnapshotRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void CopyTripleOwner(PlayerManagerScriptSnapshot& snapshot,
                                 std::uint32_t source) = 0;
    virtual void CopyPairOwner(PlayerManagerScriptSnapshot& snapshot,
                               std::uint32_t source) = 0;
    virtual void ProcessInventory(std::uint32_t entries,
                                  PlayerManagerScriptSnapshot& snapshot) = 0;
    virtual void ProcessSkills(PlayerManagerScriptSnapshot& snapshot,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t GetElementByIndex(
        const PlayerManagerScriptSnapshot& snapshot,
        std::uint32_t index) = 0;
    virtual void SetProperty(PlayerManagerScriptSnapshot& snapshot,
                             std::uint32_t index,
                             std::uint32_t value) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
};

class PlayerManagerSlotSnapshotRuntimeStub :
    public IPlayerManagerSlotSnapshotRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void CopyTripleOwner(PlayerManagerScriptSnapshot&,
                         std::uint32_t) override;
    void CopyPairOwner(PlayerManagerScriptSnapshot&,
                       std::uint32_t) override;
    void ProcessInventory(std::uint32_t,
                          PlayerManagerScriptSnapshot&) override;
    void ProcessSkills(PlayerManagerScriptSnapshot&,
                       std::uint32_t) override;
    std::uint32_t GetElementByIndex(
        const PlayerManagerScriptSnapshot&,
        std::uint32_t) override;
    void SetProperty(PlayerManagerScriptSnapshot&,
                     std::uint32_t, std::uint32_t) override;
    void Free(std::uint32_t) override;
};

class PlayerManagerSlotSnapshot final {
public:
    // IDA 0x43f5a0: build a temporary script state and copy its data out.
    [[nodiscard]] static std::uint32_t Build(
        std::uint32_t object, std::uint32_t destination,
        std::uint32_t inventory_entries,
        IPlayerManagerSlotSnapshotRuntime& runtime);
};

}  // namespace pal4
