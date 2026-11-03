#pragma once

#include "pal4/indexed_sorted_value_arrays.h"

#include <cstdint>

namespace pal4 {

class IGameDataProcessorRuntime : public IDynamicDwordArrayRuntime {
public:
    ~IGameDataProcessorRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t GetGameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t LookupPrimary(
        std::uint32_t collection_address, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t LookupSoundPrimary(
        std::uint32_t collection_address, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t LookupSecondary(
        std::uint32_t collection_address, std::uint32_t id) = 0;
};

class GameDataProcessorRuntimeStub final : public IGameDataProcessorRuntime {
public:
    std::uint32_t GetGameDatabase() override;
    std::uint32_t LookupPrimary(std::uint32_t collection_address,
                                std::uint32_t id) override;
    std::uint32_t LookupSoundPrimary(std::uint32_t collection_address,
                                     std::uint32_t id) override;
    std::uint32_t LookupSecondary(std::uint32_t collection_address,
                                  std::uint32_t id) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
};

class GameDataProcessor final {
public:
    [[nodiscard]] static std::uint32_t ProcessAndInsert(
        IndexedFilteredResultArrayState& result, std::uint32_t id,
        IGameDataProcessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessSoundAndInsert(
        IndexedFilteredResultArrayState& result, std::uint32_t id,
        IGameDataProcessorRuntime& runtime);
};

}  // namespace pal4
