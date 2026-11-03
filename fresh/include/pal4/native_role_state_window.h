#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class NativeEventHandlerArray;
struct PlayerManagerObjectState;
class IRoleStateWindowInputRuntime;
// Required host endpoints; no game page operation is silently consumed.
struct NativeRoleStateWindowServices {
    std::function<void(std::uint32_t key,std::uint32_t first,std::uint32_t second)> event;
    std::function<std::string(std::uint32_t offset)> arena_string;
    std::function<void(std::uint32_t)> tutorial;
    std::function<void(std::string_view,std::uint32_t)> sound;
    std::function<std::uint8_t(std::string_view)> warning;
    IRoleStateWindowInputRuntime* special_input = nullptr;
};
// Owns the original roleStateWindow layout and typed host dispatch over its
//0x258 image. Borrowed UI/players/database/activity/services outlive this owner.
class NativeRoleStateWindow final {
public:
    NativeRoleStateWindow(NativeCeguiUiSession&,const std::filesystem::path&,
        PlayerManagerObjectState&,const NativeGameDatabase&,NativeEventHandlerArray&,
        std::uint32_t manager,NativeRoleStateWindowServices);
    ~NativeRoleStateWindow();
    NativeRoleStateWindow(const NativeRoleStateWindow&)=delete;
    NativeRoleStateWindow& operator=(const NativeRoleStateWindow&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    std::uint32_t FrameAddress() const noexcept;
    std::uint32_t InputAddress() const noexcept;
    std::uint32_t DispatchActiveCallback(std::uint32_t member,std::uint32_t slot);
    std::uint8_t DispatchActivePredicate(std::uint32_t member,std::uint32_t slot,std::uint32_t event);
    void RethrowMouseError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
