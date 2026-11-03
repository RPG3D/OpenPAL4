#include "pal4/story_manager_binding.h"

#include <array>
#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 3> kTables = {
    "Story", "StoryDesc", "GameQuestion"};
constexpr std::array<std::uint32_t, 3> kCallbacks = {
    0x4faff0, 0x4fba10, 0x4fc1c0};

}  // namespace

std::uint32_t StoryManagerBinding::FindStory(
    std::int32_t id, IStoryManagerBindingRuntime& runtime) const {
    const auto table = runtime.ReadWord(system_ + 4);
    const auto end = runtime.ReadWord(table + 12);
    auto cursor = runtime.ReadWord(table + 8);
    auto count = static_cast<std::int32_t>(end - cursor) / 8;
    while (count > 0) {
        const auto half = count / 2;
        const auto candidate = cursor + 8 * half;
        const auto candidate_id =
            static_cast<std::int32_t>(runtime.ReadWord(candidate));
        if (candidate_id >= id) {
            count = half;
        } else {
            cursor = candidate + 8;
            count -= half + 1;
        }
    }
    if (cursor == end ||
        id < static_cast<std::int32_t>(runtime.ReadWord(cursor)))
        return 0;
    return runtime.ReadWord(table + 96) +
           36 * runtime.ReadWord(cursor + 4);
}

std::uint8_t StoryManagerBinding::RegisterHandlers(
    IStoryManagerBindingRuntime& runtime) const {
    for (std::size_t index = 0; index < 2; ++index)
        static_cast<void>(runtime.RegisterHandler(
            system_, kTables[index], kCallbacks[index]));
    return runtime.RegisterHandler(system_, kTables[2], kCallbacks[2]);
}

bool StoryManagerBinding::VerifyTables(
    IStoryManagerBindingRuntime& runtime) {
    for (std::size_t index = 0; index < kTables.size(); ++index) {
        handles_[index] = runtime.FindTable(system_, kTables[index]);
        if (handles_[index]) continue;
        const std::string message =
            "ERROR [ palDBStoryManager::verifyDataTable < the table " +
            std::string(kTables[index]) + "  doesn't exist>]";
        runtime.LogError(message);
        throw std::runtime_error(message);
    }
    return true;
}

}  // namespace pal4
