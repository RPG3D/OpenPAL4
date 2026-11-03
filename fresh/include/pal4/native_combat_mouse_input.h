#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class GameRenderState;class WindowMessageConfig;struct NativeCombatSystemImage;
class NativeCombatMouseTargets;class NativeCombatPlayerManager;
class NativeCombatMouseInput final {
public:
    using Callback=std::uint32_t(__cdecl*)(std::uint32_t);
    // previous is an actual native callable (or null), never an original EXE
    // address. Its lifetime, and all borrowed owners, extend through restoration.
    NativeCombatMouseInput(GameRenderState&,WindowMessageConfig&,NativeCombatSystemImage&,
        NativeCombatMouseTargets&,NativeCombatPlayerManager&,Callback previous=nullptr);
    ~NativeCombatMouseInput();
    // Shared5756D0 callback block; actual nonempty first Player roster required.
    void Install();
    void Restore(std::uint32_t saved_callback);
    //409670/409850 event12; canonical WindowMessageConfig+12 float pair only.
    // Errors are deferred across the WndProc/C callback ABI. Rethrow outside it.
    std::uint32_t DispatchMouse(std::uint32_t code,const std::uint32_t* data)noexcept;
    void RethrowError();
    bool Installed()const;
    std::uint32_t SavedCallback()const;
    std::uint32_t CallbackAddress()const;
    std::uint32_t DispatchCount()const;
    std::uint32_t LastResult()const;
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
