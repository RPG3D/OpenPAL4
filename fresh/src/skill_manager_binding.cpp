#include "pal4/skill_manager_binding.h"

#include <array>
#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 4> kTables = {
    "Skill", "Stunt", "Magic", "WuLing"};
constexpr std::array<std::uint32_t, 3> kCallbacks = {
    0x4f3d90, 0x4f49c0, 0x4f55a0};

}  // namespace

std::uint8_t SkillManagerBinding::RegisterHandlers(
    ISkillManagerBindingRuntime& runtime) const {
    for (std::size_t index = 0; index < 2; ++index)
        static_cast<void>(runtime.RegisterHandler(
            system_, kTables[index], kCallbacks[index]));
    return runtime.RegisterHandler(system_, kTables[2], kCallbacks[2]);
}

bool SkillManagerBinding::VerifyTables(
    ISkillManagerBindingRuntime& runtime) {
    for (std::size_t index = 0; index < kTables.size(); ++index) {
        handles_[index] = index < 3
                              ? runtime.FindTable(system_, kTables[index])
                              : runtime.FindPropertyString(system_, kTables[index]);
        if (handles_[index] != 0) continue;
        const std::string message =
            "ERROR [ palDBSkillManager::verifyDataTable < the table " +
            std::string(kTables[index]) + "  doesn't exist>]";
        runtime.LogError(message);
        throw std::runtime_error(message);
    }
    return true;
}

}  // namespace pal4
