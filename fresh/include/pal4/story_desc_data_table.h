#pragma once

#include "pal4/story_data_table.h"

#include <cstdint>

namespace pal4 {

class StoryDescDataTable final {
public:
    // IDA 0x4fba10: construct the 140-byte StoryDesc table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, IStoryDataTableRuntime& runtime);
    // IDA 0x4fbae0: release StoryDesc rows, properties and base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t table, IStoryDataTableDestroyRuntime& runtime);
    // IDA 0x4fbc20: parse and insert or replace a StoryDesc row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IStoryDataRowRuntime& runtime);
    // IDA 0x4fbea0: convert and append a StoryDesc row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IStoryDataRowRuntime& runtime);
    // IDA 0x4fc040: set dirty byte before StoryDesc refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IStoryDataRefreshRuntime& runtime);
    // IDA 0x4fc050: reserve StoryDesc records and key index.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        IStoryDataCapacityRuntime& runtime);
    // IDA 0x4fe1a0: refresh StoryDesc field bindings on a count mismatch.
    [[nodiscard]] static std::uint32_t RefreshBindings(
        std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime);
};

}  // namespace pal4
