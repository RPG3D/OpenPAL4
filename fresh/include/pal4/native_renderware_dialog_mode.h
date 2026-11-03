#pragma once

#include <cstdint>

namespace pal4 {

struct NativeRenderWareDialogModeResult final {
    std::uint32_t requested_sdk_version = 0;
    std::uint32_t forwarded_sdk_version = 0;
    std::uint32_t create_device_calls = 0;
    std::uint32_t present_flags = 0;
    std::uint32_t swap_effect = 0;
    std::uint32_t multisample_type = 0;
    std::int32_t dialog_hresult = 0;
};

// This process-wide D3D9 import/vtable adaptation must run on the engine thread.
// Prepare before RwEngineOpen; enable after device selection; finish after Start.
class NativeRenderWareDialogMode final {
public:
    NativeRenderWareDialogMode() = default;
    NativeRenderWareDialogMode(const NativeRenderWareDialogMode&) = delete;
    NativeRenderWareDialogMode& operator=(const NativeRenderWareDialogMode&) = delete;
    ~NativeRenderWareDialogMode();

    [[nodiscard]] bool PrepareBeforeOpen();
    [[nodiscard]] bool EnableBeforeStart(bool enabled);
    [[nodiscard]] bool FinishAfterStart();
    [[nodiscard]] bool Cancel();
    [[nodiscard]] const NativeRenderWareDialogModeResult& Result() const {
        return result_;
    }

private:
    static void* __stdcall CreateD3D9(std::uint32_t sdk_version);
    static long __stdcall CreateDevice(
        void* d3d, std::uint32_t adapter, std::uint32_t type, void* window,
        std::uint32_t behavior, void* present, void* output);
    [[nodiscard]] bool PatchPointer(std::uintptr_t* slot, std::uintptr_t value);

    std::uintptr_t* create9_import_ = nullptr;
    std::uintptr_t original_create9_ = 0;
    std::uintptr_t* create_device_slot_ = nullptr;
    std::uintptr_t original_create_device_ = 0;
    std::uint32_t owner_thread_ = 0;
    bool enabled_ = false;
    bool wrong_thread_ = false;
    NativeRenderWareDialogModeResult result_{};
};

}  // namespace pal4
