#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class NativeEventHandlerArray;
// Owns the original SettleSelectWindow layout, TimerFrame state and navigation.
class NativeInnSelectorWindow final {
public:
    NativeInnSelectorWindow(NativeCeguiUiSession&,const std::filesystem::path&,
        const NativeGameDatabase&,NativeEventHandlerArray&,std::uint32_t manager);
    ~NativeInnSelectorWindow();
    void Show(std::uint32_t expanded);
    std::uint32_t FrameAddress()const noexcept;
    std::uint32_t RootAddress()const noexcept;
    std::uint32_t Window(std::uint32_t offset)const;
    std::int32_t Result()const;
    bool Visible()const noexcept;
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    void RethrowError();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
