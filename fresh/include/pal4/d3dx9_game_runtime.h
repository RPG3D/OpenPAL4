#pragma once

#include "pal4/d3dx9_native.h"
#include "pal4/effect_script_vm.h"
#include "pal4/pal_font_backend.h"
#include "pal4/screen_effect_script.h"

namespace pal4 {

class D3DX9FontRuntime final : public IPalFontBackendRuntime {
public:
    explicit D3DX9FontRuntime(IDirect3DDevice9* device) : device_(device) {}
    std::uint32_t CurrentD3DDevice() override;
    std::int32_t CreateFromDescription(std::uint32_t device, std::int32_t height,
        std::int32_t width, std::int32_t weight, std::int32_t mip_levels,
        std::int32_t italic, const std::array<std::uint8_t, 4>& options,
        std::uint32_t face_name, std::uint32_t output_font) override;
private:
    IDirect3DDevice9* device_; // Borrowed; the host owns device lifetime.
};

// The host supplies game file/package/device services. These implementations
// replace only the audited D3DX boundary; no private compiler classes remain.
class D3DX9EffectRuntime : public IEffectScriptVmRuntime {
public:
    std::uint32_t LoadAndExecute(std::uint32_t device, std::uint32_t buffer,
        std::uint32_t bytes, std::uint32_t defines, std::uint32_t include,
        std::uint32_t flags, std::uint32_t pool, std::uint32_t& effect,
        std::uint32_t& errors) override;
    std::string_view ErrorText(std::uint32_t errors) override;
    void ReleaseVm(std::uint32_t effect) override;
    std::uint32_t InvokeVmSlot(std::uint32_t effect, std::uint32_t offset) override;
};

class D3DX9ScreenEffectRuntime : public IScreenEffectRuntime {
public:
    void SelectTechnique(std::uint32_t effect, std::string_view name) override;
    void BindTexture(std::uint32_t effect, std::string_view name,
                     std::uint32_t texture) override;
    void ExecuteTechnique(std::uint32_t effect, std::uint32_t& passes,
                          std::uint32_t flags) override;
    void LoadTexture(std::uint32_t device, std::uint32_t data,
                     std::uint32_t bytes, std::uint32_t& texture) override;
    void ReleaseEffectResource(std::uint32_t resource) override;
    std::uint32_t InvokeRenderSlot(std::uint32_t effect, std::uint32_t offset) override;
    std::uint32_t InvokeParameterSlot(std::uint32_t effect, std::uint32_t pass) override;
    std::uint32_t ReleaseScript(std::uint32_t effect) override;
    std::uint32_t LoadScript(std::uint32_t device, std::uint32_t buffer,
        std::uint32_t bytes, std::uint32_t defines, std::uint32_t include,
        std::uint32_t flags, std::uint32_t pool, std::uint32_t& effect,
        std::uint32_t& errors) override;
    std::string_view ErrorText(std::uint32_t errors, std::string_view label) override;
};

} // namespace pal4
