#pragma once
#include "pal4/d3dx9_game_runtime.h"
#include <memory>

namespace pal4 {
class NativePkFileManagerRuntime;
// File/package and lifetime boundary for the shared screen-effect loaders.
// The package manager and device are borrowed and must outlive this adapter.
// D3DX owns effects; their ScreenEffectState owner must call shared cleanup.
class NativeScreenEffectScriptRuntime final : public D3DX9ScreenEffectRuntime {
public:
    NativeScreenEffectScriptRuntime(NativePkFileManagerRuntime&,IDirect3DDevice9*);
    ~NativeScreenEffectScriptRuntime() override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void Reset(ScreenEffectState&) override;
    bool CheckCapability(std::uint32_t) override;
    void ProcessPackageScript() override;
    void CleanupScriptEnvironment(std::uint32_t&) override;
    std::uint32_t OpenFile(std::string_view,std::string_view) override;
    void SeekFile(std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t FileSize(std::uint32_t) override;
    std::uint32_t AllocateBuffer(std::uint32_t) override;
    void ReadFile(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override;
    void CloseFile(std::uint32_t) override;
    std::uint32_t CurrentD3DDevice() override;
    std::uint32_t LoadScript(std::uint32_t,std::uint32_t,std::uint32_t,
        std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,
        std::uint32_t&,std::uint32_t&) override;
    void ShowError(std::string_view,std::uint32_t) override;
    void ReleaseEnvironmentOnLoadError(std::uint32_t) override;
    void FreeBuffer(std::uint32_t) override;
    [[nodiscard]] bool LoadScopeActive() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
