#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
namespace pal4 {
class NativeCeguiUiSession;
// Real frameToolbar/decorator/gameInfo owners, shared by native System pages.
// The UI/DLL and data/event endpoints outlive this owner; close before UI Stop.
struct NativeSystemMenuDecorationServices {
    std::function<void(std::uint32_t key,std::uint32_t first,std::uint32_t second)> event;
    std::function<std::int32_t()> money;
    std::function<std::string(std::uint32_t)> game_string;
    std::function<std::uint32_t()> elapsed_seconds,clock_offset_seconds;
};
class NativeSystemMenuDecorations final {
public:
    NativeSystemMenuDecorations(NativeCeguiUiSession&,const std::filesystem::path& root,
        std::uint32_t manager,NativeSystemMenuDecorationServices);
    ~NativeSystemMenuDecorations();
    NativeSystemMenuDecorations(const NativeSystemMenuDecorations&)=delete;
    NativeSystemMenuDecorations& operator=(const NativeSystemMenuDecorations&)=delete;
    bool HandleEvent(std::uint32_t key,std::uint32_t first,std::uint32_t second);
    void UpdateClock();
    void RethrowClickError();
    void Close();
    std::uint32_t ToolbarFrame() const noexcept;
    std::uint32_t InfoFrame() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
