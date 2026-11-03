#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
// Original496C20 layout, callbacks and three-tab navigation storage. The
// required reset service must execute the real list/reset composition.
class NativeSmithWindowLayout final {
public:
    using Handler=std::function<bool(std::uint32_t,std::uint32_t,std::uint32_t)>;
    using Reset=std::function<void(NativeSmithWindowLayout&)>;
    using Retire=std::function<void()>;
    NativeSmithWindowLayout(NativeCeguiUiSession&,const std::filesystem::path&,Handler,Reset,Retire);
    ~NativeSmithWindowLayout();
    NativeSmithWindowLayout(const NativeSmithWindowLayout&)=delete;
    NativeSmithWindowLayout& operator=(const NativeSmithWindowLayout&)=delete;
    std::uint32_t FrameAddress()const noexcept;
    std::uint32_t RootAddress()const noexcept;
    std::uint32_t& Word(std::uint32_t offset);
    void RethrowEventError();
    bool InCallback()const noexcept;
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
