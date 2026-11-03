#pragma once

#include "pal4/game_data_field_registry_runtime.h"

#include <cstdint>

namespace pal4 {

enum class GameDataThreeFieldSchema { NameMemo, ChapterMemo };

class GameDataThreeFieldRegistry final {
public:
    explicit GameDataThreeFieldRegistry(GameDataThreeFieldSchema schema)
        : schema_(schema) {}

    // IDA 0x4fa130 / 0x4fa460: initialize and return a guarded field collection.
    [[nodiscard]] std::uint32_t Initialize(
        IGameDataFieldRegistryRuntime& runtime);
    // IDA 0x4fa430 / 0x4fa760: clean up the selected collection.
    void Cleanup(IGameDataFieldRegistryRuntime& runtime) const;
    // IDA 0x4fa440 / 0x4fa770: initialize, then process an event.
    [[nodiscard]] std::uint8_t ProcessEvent(
        std::uint32_t input, IGameDataFieldRegistryRuntime& runtime);

private:
    [[nodiscard]] std::uint32_t CollectionAddress() const;
    [[nodiscard]] std::uint32_t CleanupAddress() const;

    GameDataThreeFieldSchema schema_;
    bool collection_initialized_ = false;
    bool properties_initialized_ = false;
};

}  // namespace pal4
