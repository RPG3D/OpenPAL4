#pragma once
#include "pal4/rw_texture_read_hook.h"

namespace pal4 {
// One callback per RenderWare engine. Use on the engine thread, after Start;
// destroy this adapter before Stop. Borrowed state must outlive this adapter.
// Install once: the original repeated-install self-chain is not a valid setup.
class NativeRwTextureReadHookRuntime final : public IRwTextureReadHookRuntime {
public:
    explicit NativeRwTextureReadHookRuntime(RwTextureReadHookState& state);
    ~NativeRwTextureReadHookRuntime() override;
    NativeRwTextureReadHookRuntime(const NativeRwTextureReadHookRuntime&)=delete;
    NativeRwTextureReadHookRuntime& operator=(const NativeRwTextureReadHookRuntime&)=delete;
    std::uint32_t GetReadCallback() override;
    std::uint32_t SetReadCallback(std::uint32_t callback) override;
    std::uint32_t ReadDds(const char* base_name,std::uint32_t mask_address) override;
    std::uint32_t ReadOriginal(std::uint32_t callback,const char* name,std::uint32_t mask_address) override;
    std::uint32_t Dispatch(const char* name,const char* mask);
private:
    RwTextureReadHookState& state_;
    std::uint32_t previous_native_{};
};
}
