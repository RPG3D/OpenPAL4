#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

enum class GameDataPropertyKind {
    text,
    numeric,
    combat_position,
    race,
};

struct GameDataProperty {
    std::uint32_t id = 0;
    std::uint32_t source_address = 0;
    GameDataPropertyKind kind = GameDataPropertyKind::text;
    std::string name;
    std::uint32_t source_count = 0;

    bool operator==(const GameDataProperty&) const = default;
};

class IGameDataStructureRegistryRuntime {
public:
    virtual ~IGameDataStructureRegistryRuntime() = default;
    virtual void InitializeGlobalStorage() = 0;
    virtual void RegisterGlobalCleanup() = 0;
    virtual void CleanupGlobalStorage() = 0;
    virtual void SeedCollection(std::vector<GameDataProperty>& entries) = 0;
    [[nodiscard]] virtual std::string SourceName(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ErrorSuffix() = 0;
    virtual void Sort(std::vector<GameDataProperty>& entries) = 0;
};

class GameDataStructureRegistry final {
public:
    // IDA 0x4c8020: one-time global storage and 48 property registrations.
    [[nodiscard]] const std::vector<GameDataProperty>& Initialize(
        IGameDataStructureRegistryRuntime& runtime);
    // IDA 0x4c87f0: CRT callback delegates to collection cleanup.
    void Cleanup(IGameDataStructureRegistryRuntime& runtime);

    [[nodiscard]] bool GlobalsInitialized() const { return globals_initialized_; }
    [[nodiscard]] bool EntriesInitialized() const { return entries_initialized_; }
    [[nodiscard]] const std::vector<GameDataProperty>& Entries() const {
        return entries_;
    }

private:
    bool globals_initialized_ = false;
    bool entries_initialized_ = false;
    std::vector<GameDataProperty> entries_;
};

}  // namespace pal4
