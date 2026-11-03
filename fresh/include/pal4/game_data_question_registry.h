#pragma once

#include "pal4/game_data_field_registry_runtime.h"

#include <cstdint>

namespace pal4 {

class GameDataQuestionRegistry final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8ef2dc;
    static constexpr std::uint32_t kCleanupAddress = 0x4fad80;

    // IDA 0x4fa790: build the guarded seven-field question schema.
    [[nodiscard]] std::uint32_t Initialize(
        IGameDataFieldRegistryRuntime& runtime);
    // IDA 0x4fad80: release the static collection.
    void Cleanup(IGameDataFieldRegistryRuntime& runtime) const;
    // IDA 0x4fad90: initialize before dispatching an event.
    [[nodiscard]] std::uint8_t ProcessEvent(
        std::uint32_t input, IGameDataFieldRegistryRuntime& runtime);

private:
    bool collection_initialized_ = false;
    bool properties_initialized_ = false;
};

}  // namespace pal4
