#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;class NativeMagicWindowLayout;class NativeMagicWindowPresentation;
class NativeMagicWindowRecords;class NativeEventHandlerArray;class NativePalGameIVTextureStorage;
struct PlayerManagerObjectState;
struct NativeMagicInteractionServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
    // The gameplay owner supplies actual learning/cast controllers. Never make
    // an absent endpoint look successful merely because the page can render.
    std::function<void(std::uint32_t clicked_button)> magic_point;
    std::function<void(std::uint32_t event_payload,std::uint32_t stunt_record)> stunt_select;
    std::function<void(std::uint32_t key)> use_stunt;
};
// Borrows layout/presentation/game services. Close before destroying any of them;
// route layout subscribers and the shared global activity array through this owner.
class NativeMagicWindowInteraction final {
public:
    NativeMagicWindowInteraction(NativeCeguiUiSession&,NativeMagicWindowLayout&,NativeMagicWindowPresentation&,
        PlayerManagerObjectState&,const NativeMagicWindowRecords&,NativePalGameIVTextureStorage&,
        NativeEventHandlerArray&,std::uint32_t manager,NativeMagicInteractionServices);
    ~NativeMagicWindowInteraction();
    NativeMagicWindowInteraction(const NativeMagicWindowInteraction&)=delete;
    NativeMagicWindowInteraction& operator=(const NativeMagicWindowInteraction&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    bool FromDll(std::uint32_t owner,std::uint32_t handler,std::uint32_t event);
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    void RethrowEventError();
    void Close();
private:
    class Runtime;std::unique_ptr<Runtime> runtime_;
};
}
