#pragma once
#include <cstdint>
#include <memory>
#include <string_view>
struct IDirect3DDevice9;
namespace pal4 {
class NativePkFileManagerRuntime;
// Owns the original MatFX effect through the audited public D3DX boundary.
// The package manager and device must outlive this session. This prepares a
// dependency for Scene and Combat.
class NativeMatFxScriptSession final {
public:
    NativeMatFxScriptSession(NativePkFileManagerRuntime&, IDirect3DDevice9*);
    ~NativeMatFxScriptSession();
    NativeMatFxScriptSession(const NativeMatFxScriptSession&) = delete;
    NativeMatFxScriptSession& operator=(const NativeMatFxScriptSession&) = delete;
    [[nodiscard]] bool Load(std::string_view path = "gamedata\\MatFX\\Shader\\ltMap.fx");
    void Reset();
    [[nodiscard]] std::uint32_t Effect() const noexcept;
    [[nodiscard]] bool LoadScopeActive() const noexcept;
    [[nodiscard]] std::uint32_t OnLostDevice();
    [[nodiscard]] std::uint32_t OnResetDevice();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
