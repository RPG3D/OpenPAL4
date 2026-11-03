#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string_view>
#include <array>
#include <functional>
namespace pal4 {
class NativeCeguiUiSession;
class NativeGameTimerQueue;
class NativeCombatHintsRuntime;
class NativeMagicWindowRecords;
class NativeGameDatabase;
class NativeInventorySession;
// Owns two ordinary and four Ending hint layouts and their host clock strings. Borrowed UI,
// timer queue and actual frame-manager image outlive Close.
class NativeCombatHints final {
public:
    NativeCombatHints(NativeCeguiUiSession&,NativeGameTimerQueue&,std::uint32_t frame_manager);
    ~NativeCombatHints();
    std::uint32_t Ensure(std::uint32_t variant);
    void Cleanup(std::uint32_t window);
    std::uint32_t Root(std::uint32_t variant)const;
    std::string_view Clock(std::uint32_t variant)const;
    std::size_t Count()const;
    // Separate original CombatHintWindow instances use the supplied prefix
    // verbatim (549E60 -> 4BAEC0), without the default trailing slash.
    std::uint32_t ReserveLevelUp(std::uint32_t slot);
    std::uint32_t ConstructLevelUp(std::uint32_t address,std::string_view prefix);
    void ReleaseLevelUp(std::uint32_t address);
    void ShowCastMagic(std::uint32_t event,const NativeMagicWindowRecords&);
    void ShowStunt(std::uint32_t event,const NativeMagicWindowRecords&);
    void ShowUseProperty(std::uint32_t event,const NativeInventorySession&);
    void ShowProtect(const NativeGameDatabase&);
    // actor is borrowed from the console visual owner after registry validation.
    void ShowMonsterFlee(std::uint32_t actor,bool failed,const NativeGameDatabase&);
    void ShowMoney(std::uint32_t event,const NativeGameDatabase&,
        const std::function<std::uint32_t()>& acquire_combat,
        const std::function<void(std::string_view,std::uint32_t)>& play_sound);
    void ShowPickedProperty(std::uint32_t event,const NativeInventorySession&,const NativeGameDatabase&,
        const std::function<std::uint32_t()>& acquire_combat,
        const std::function<void(std::string_view,std::uint32_t)>& play_sound);
    void ShowPickUpFailed(const NativeGameDatabase&);
    void ShowCombatText(std::string_view,const std::array<float,2>& position,float duration);
    void ShowNotice(std::string_view,const std::array<float,2>& position,float duration);
    void ShowLevelUp(std::uint32_t address,std::string_view,const std::array<float,2>& position,float duration);
    bool HandleTimer(std::uint32_t owner,std::uint32_t method);
    void SetLevelUpFlag(std::uint32_t address,std::uint8_t flag);
    std::uint32_t LevelUpRoot(std::uint32_t slot)const;
    std::string_view LevelUpClock(std::uint32_t slot)const;
    std::size_t LevelUpCount()const;
    void Close();
private:
    void ShowGameStringHint(std::uint32_t actor,std::uint32_t key,const NativeGameDatabase&,
        std::uint32_t event=0u,const std::function<std::uint32_t()>& acquire_combat={},
        const std::function<void(std::string_view,std::uint32_t)>& play_sound={},
        const NativeInventorySession* inventory=nullptr);
    void ShowRecordText(std::uint32_t event,std::uint32_t record,std::uint32_t offset,std::string_view text);
    friend class NativeCombatHintsRuntime;
    class Impl;std::unique_ptr<Impl> impl_;
};
}
