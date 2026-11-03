#pragma once
#include "pal4/game_manager_reset.h"
#include <memory>
#include <cstddef>
namespace pal4 {
class NativeBaseObjectMemory;
class NativePkFileManagerRuntime;
// Native facets for shared CG initialization, dispatch, reset and render passes.
// Host STL owns the handler catalog; the arena's legacy empty tree stays empty.
// Bound handlers: OldTV, Invert and ten-camera timed trails. Other handlers fail explicitly.
class NativeScreenEffectSession final : public IGameManagerResetRuntime {
public:
    NativeScreenEffectSession(NativeBaseObjectMemory&, NativePkFileManagerRuntime&, ScreenEffectDispatcherState&);
    ~NativeScreenEffectSession();
    void BindCamera(std::uint32_t);
    void UnbindCamera();
    bool Initialize();
    [[nodiscard]] std::size_t HandlerCount() const;
    void HandleScript(std::int32_t key);
    bool CapturingEnabled() const;
    std::uint32_t BeginFrame();
    void EndFrame();
    void UpdateRenderParameters(double seconds);
    void RenderPasses(std::uint32_t argument);
    void CallHandler(std::uint32_t,std::uint32_t) override;
    std::uint32_t FirstNode(std::uint32_t) override;
    std::uint32_t NodeObject(std::uint32_t) override;
    void InvokeShutdown(std::uint32_t,std::uint32_t) override;
    void DeleteObject(std::uint32_t,std::uint8_t) override;
    std::uint32_t RemoveNode(ScreenEffectDispatcherState&,std::uint32_t) override;
    void CleanupCamera(std::uint32_t) override;
    std::uint32_t DestroyCameraRecord(std::uint32_t) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
