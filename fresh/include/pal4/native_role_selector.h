#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
class NativeEventHandlerArray;
struct PlayerManagerObjectState;
// Owns original RoleSelectorWindow.xml, four role cards, modal navigation and
// copied choice callbacks. UI/player/activity owners outlive Close.
class NativeRoleSelector final {
public:
    using Choice=std::function<void(std::uint32_t role,std::uint32_t context)>;
    NativeRoleSelector(NativeCeguiUiSession&,const std::filesystem::path&,
        PlayerManagerObjectState&,NativeEventHandlerArray&,std::uint32_t manager);
    ~NativeRoleSelector();
    NativeRoleSelector(const NativeRoleSelector&)=delete;
    NativeRoleSelector& operator=(const NativeRoleSelector&)=delete;
    void Show(std::uint32_t context,Choice);
    void Hide();
    void Refresh();
    void Close();
    void RethrowEventError();
    bool Visible()const noexcept;
    std::uint32_t FrameAddress()const noexcept;
    std::uint32_t CardButton(std::uint32_t card)const;
    std::uint32_t CardRole(std::uint32_t card)const;
    std::uint32_t VisibleCardCount()const noexcept;
    std::uint64_t SelectionGeneration()const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
