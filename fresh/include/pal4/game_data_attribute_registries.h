#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pal4/game_data_structure_registry.h"

namespace pal4 {

enum class GameDataAttributeGroup {
    conditions,
    parameters,
};

class IGameDataAttributeRegistriesRuntime {
public:
    virtual ~IGameDataAttributeRegistriesRuntime() = default;
    virtual void InitializeStorage(GameDataAttributeGroup group,
                                   std::uint8_t first_seed,
                                   std::uint8_t second_seed) = 0;
    virtual void RegisterCleanup(GameDataAttributeGroup group) = 0;
    virtual void CleanupStorage(GameDataAttributeGroup group) = 0;
    virtual void AppendRecord(GameDataAttributeGroup group,
                              const GameDataProperty& record,
                              std::uint8_t seed) = 0;
    [[nodiscard]] virtual std::string SourceName(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ErrorSuffix() = 0;
    virtual void Sort(GameDataAttributeGroup group,
                      std::vector<GameDataProperty>& records) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessCollection(
        std::uint32_t object, std::uint32_t collection) = 0;
};

class GameDataAttributeRegistries final {
public:
    // IDA 0x4cb640/0x4cb8f0/0x4cb900: conditions collection.
    [[nodiscard]] std::uint32_t InitializeConditions(
        std::uint8_t first_seed, std::uint8_t second_seed,
        IGameDataAttributeRegistriesRuntime& runtime);
    void CleanupConditions(IGameDataAttributeRegistriesRuntime& runtime);
    [[nodiscard]] std::uint8_t ProcessConditions(
        std::uint32_t object, std::uint8_t first_seed,
        std::uint8_t second_seed, IGameDataAttributeRegistriesRuntime& runtime);

    // IDA 0x4cb920/0x4cbdb0/0x4cbde0: parameters collection.
    [[nodiscard]] std::uint32_t InitializeParameters(
        std::uint8_t first_seed, std::uint8_t second_seed,
        IGameDataAttributeRegistriesRuntime& runtime);
    void CleanupParameters(IGameDataAttributeRegistriesRuntime& runtime);
    [[nodiscard]] std::uint8_t ProcessParameters(
        std::uint32_t object, std::uint8_t first_seed,
        std::uint8_t second_seed, IGameDataAttributeRegistriesRuntime& runtime);

    [[nodiscard]] const std::vector<GameDataProperty>& Conditions() const {
        return conditions_.records;
    }
    [[nodiscard]] const std::vector<GameDataProperty>& Parameters() const {
        return parameters_.records;
    }

private:
    struct State {
        bool storage_initialized = false;
        bool records_initialized = false;
        std::vector<GameDataProperty> records;
    };

    [[nodiscard]] static std::uint32_t Initialize(
        State& state, GameDataAttributeGroup group,
        std::uint8_t first_seed, std::uint8_t second_seed,
        IGameDataAttributeRegistriesRuntime& runtime);

    State conditions_;
    State parameters_;
};

}  // namespace pal4
