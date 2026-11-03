#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
// Original TradeWindow constructor/control/event composition. Data, effects,
// activity and transaction controllers borrow this image and retire first.
class NativeTradeWindowLayout final {
public:
    using Handler=std::function<bool(std::uint32_t method,std::uint32_t event)>;
    using Retire=std::function<void()>;
    NativeTradeWindowLayout(NativeCeguiUiSession&,const std::filesystem::path&,
        std::uint32_t manager,Handler,Retire);
    ~NativeTradeWindowLayout();
    NativeTradeWindowLayout(const NativeTradeWindowLayout&)=delete;
    NativeTradeWindowLayout& operator=(const NativeTradeWindowLayout&)=delete;
    std::uint32_t FrameAddress()const noexcept;
    std::uint32_t RootAddress()const noexcept;
    std::uint32_t& Word(std::uint32_t offset);
    bool Event(std::uint32_t key,const std::array<std::uint32_t,2>& payload);
    void BindParent(bool,std::uint32_t parent=0u);
    bool InCallback()const noexcept;
    void RethrowEventError();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
