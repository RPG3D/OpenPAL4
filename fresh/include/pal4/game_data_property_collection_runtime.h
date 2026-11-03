#pragma once

#include "pal4/game_data_structure_registry.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

class IGameDataPropertyCollectionRuntime {
public:
    virtual ~IGameDataPropertyCollectionRuntime() = default;
    virtual void InitializeStorage(std::uint8_t first_seed,
                                   std::uint8_t second_seed) = 0;
    virtual void RegisterCleanup() = 0;
    virtual void CleanupStorage() = 0;
    virtual void ResetTemporaryName(std::uint8_t seed) = 0;
    virtual void ReleaseTemporaryName() = 0;
    [[nodiscard]] virtual std::string SourceName(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ErrorSuffix() = 0;
    virtual void AppendRecord(const GameDataProperty& record) = 0;
    virtual void Sort(std::vector<GameDataProperty>& records) = 0;
};

}  // namespace pal4
