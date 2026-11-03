#include "pal4/combat_effect_config_keys.h"
#include "pal4/combat_animation_command_processor.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

constexpr std::uint32_t kSlotBase = 96u;
constexpr std::uint32_t kSlotStride = 24u;

bool StartsWith(const std::string& value, const std::string_view prefix) {
    return value.size() >= prefix.size() &&
           value.compare(0, prefix.size(), prefix) == 0;
}

void TrimCharacters(std::string& value, const std::string_view characters) {
    const auto begin = value.find_first_not_of(characters);
    if (begin == std::string::npos) {
        value.clear();
        return;
    }
    const auto end = value.find_last_not_of(characters);
    value = value.substr(begin, end - begin + 1);
}

int CommandIndex(std::string& command, const std::string_view prefix) {
    TrimCharacters(command, prefix);
    return std::atoi(command.c_str());
}

struct TemporaryEffect final {
    ICombatAnimationCommandRuntime& runtime;
    CombatEffectHandle handle;
    ~TemporaryEffect() { runtime.ResetTemporaryEffect(handle); }
};

}  // namespace

std::uint32_t CombatAnimationCommandProcessor::Process(
    const std::uint32_t task, const std::string_view message_name,
    ICombatAnimationCommandRuntime& runtime) {
    const auto actor = runtime.Actor(task);
    const auto entry = runtime.FindAnimationEntry(actor, message_name);
    const auto clump = runtime.ChildClump(actor);
    if (!clump)
        return clump;

    std::string command(message_name);
    std::transform(command.begin(), command.end(), command.begin(),
                   [](const unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });

    if (command.size() > 3u && StartsWith(command, "efs")) {
        const auto index = CommandIndex(command, "efs");
        if (index >= 0 && index < 5) {
            const auto slot = task + kSlotBase + kSlotStride * index;
            if (runtime.SlotActive(slot))
                runtime.ReleaseSlot(slot);
        }
    } else if (entry && command.size() > 2u && StartsWith(command, "ef")) {
        const auto index = CommandIndex(command, "ef");
        if (index >= 0 && index < 5) {
            const auto resource = runtime.Attribute(entry, "ef");
            const auto bind = runtime.Attribute(entry, "bind");
            const auto handle = runtime.CreateEffect(resource);
            const TemporaryEffect temporary{runtime, handle};
            if (runtime.EffectActive(handle)) {
                const auto slot = task + kSlotBase + kSlotStride * index;
                if (runtime.SlotActive(slot))
                    runtime.ReleaseSlot(slot);
                runtime.StoreSlot(slot, handle, bind);
                runtime.SetCallback(handle, CombatEffectConfigKeys::AttachedCast, task);
                if (runtime.TaskTarget(task)) {
                    const auto target = runtime.TaskTarget(task);
                    if (target)
                        runtime.SetTarget(handle, "T1", target + 724u);
                }
                std::string binding_key(bind);
                std::transform(binding_key.begin(),binding_key.end(),binding_key.begin(),
                    [](const unsigned char c){return static_cast<char>(std::tolower(c));});
                if (binding_key == "hook_ef_pos") {
                    runtime.AttachActorTransform(handle, actor);
                } else {
                    runtime.ApplyBindingRotation(handle);
                    const auto frame = runtime.EffectFrame(handle);
                    runtime.ProcessBinding(runtime.ChildClump(actor), bind,
                                           frame);
                }
                runtime.UpdateEffect(handle);
            }
        }
    } else if (entry && StartsWith(command, "tms")) {
        runtime.SetTimeScale(runtime.FloatAttribute(entry, "scale", 1.0f));
    } else if (StartsWith(command, "rtm")) {
        runtime.SetTimeScale(1.0f);
    } else if (entry && StartsWith(command, "cg")) {
        (void)runtime.IntAttribute(entry, "type", 1);
    } else if (!StartsWith(command, "rcg")) {
        if (command == "wphide")
            runtime.SetHandsActive(actor, false);
        else if (command == "wpshow")
            runtime.SetHandsActive(actor, true);
    }
    return runtime.CleanupCommand(command);
}

}  // namespace pal4
