#pragma once
#include "pal4/magic_window.h"
#include "pal4/magic_window_panel.h"
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;class NativeGameDatabase;
// Owns the original layout, DLL subscribers and category navigation storage.
// Semantic state is kept separately from the legacy image used for member IDs;
// original EXE vtable addresses are identifiers and are never invoked.
class NativeMagicWindowLayout final {
public:
    using Handler=std::function<bool(std::uint32_t,std::uint32_t,std::uint32_t)>;
    NativeMagicWindowLayout(NativeCeguiUiSession&,const std::filesystem::path&,
        const NativeGameDatabase&,std::uint32_t manager,Handler);
    ~NativeMagicWindowLayout();
    NativeMagicWindowLayout(const NativeMagicWindowLayout&)=delete;
    NativeMagicWindowLayout& operator=(const NativeMagicWindowLayout&)=delete;
    MagicWindowState& State();
    const MagicWindowPanelState& Panel()const;
    std::uint32_t FrameAddress()const noexcept;
    std::uint32_t RootAddress()const noexcept;
    std::uint32_t& Word(std::uint32_t offset);
    void DispatchCommand(std::string_view,std::uint32_t);
    void RethrowEventError();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
