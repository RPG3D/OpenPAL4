#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
namespace pal4 {
class NativeScriptEventRouter;
// Shared579BD0/579D00/579ED0 algorithms use these public game services.
// Text callbacks resolve existing game text; temporary text uses host std::string.
struct NativeCombatEndSoundServices {
    std::function<std::uint32_t()> acquire_combat;
    std::function<std::uint32_t(std::uint32_t)> read_word;
    std::function<bool(std::uint32_t)> inactive;
    std::function<std::string(std::uint32_t)> text;
    std::function<void(std::string_view,float,bool)> special_music;
    std::function<void(std::string_view)> restore_music;
    std::function<std::uint32_t(std::string_view,std::uint32_t)> play_sound;
};
class NativeCombatEndSoundEvents final {
public:
    NativeCombatEndSoundEvents(std::uint32_t combat,std::uint32_t& slot,
                              NativeScriptEventRouter&,NativeCombatEndSoundServices);
    ~NativeCombatEndSoundEvents();
    NativeCombatEndSoundEvents(const NativeCombatEndSoundEvents&)=delete;
    NativeCombatEndSoundEvents& operator=(const NativeCombatEndSoundEvents&)=delete;
    void Close();
    std::uint32_t Address() const;
    std::size_t SubscriptionCount() const;
    std::size_t TemporaryTexts() const;
    // Reuse original579ED0 with a host-owned input copy for actor sound callers.
    std::uint32_t PlayEffect(std::string_view resource,std::uint32_t mode=1u);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
