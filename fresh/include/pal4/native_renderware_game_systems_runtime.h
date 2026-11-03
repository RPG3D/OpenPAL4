#pragma once

#include "pal4/game_systems_initializer.h"
#include "pal4/native_renderware_dialog_mode.h"
#include "pal4/native_image_handler_runtime.h"
#include "pal4/native_tga_image_reader.h"
#include "pal4/win32_input_runtime.h"
#include "pal4/video_device_selector.h"
#include "pal4/virtual_file_system_initializer.h"

#include <functional>
#include <memory>
#include <utility>

struct RwMemoryFunctions;

namespace pal4 {

// Connects the reviewed game startup sequence to the native RenderWare SDK.
// Services outside this boundary are supplied by the caller.
class NativeRenderWareGameSystemsRuntime final : public IGameSystemsRuntime {
public:
    NativeRenderWareGameSystemsRuntime(
        IGameSystemsRuntime& services, GameConfigWords& config,
        GameRenderState& state, IVideoDeviceRuntime& video_runtime,
        IVirtualFileSystemRuntime& file_system_runtime,
        RwImageCallBackRead tga_reader = ReadNativeGameTga,
        const RwMemoryFunctions* memory_functions = nullptr,
        std::function<void()> before_file_system_close = {})
        : services_(services), config_(config), state_(state),
          video_runtime_(video_runtime), file_system_runtime_(file_system_runtime),
          tga_reader_(tga_reader), memory_functions_(memory_functions),
          before_file_system_close_(std::move(before_file_system_close)) {}
    ~NativeRenderWareGameSystemsRuntime() override;

    void InitializeAllocator(std::uint32_t table_address) override;
    [[nodiscard]] std::uint32_t EngineInit(
        std::uint32_t table_address, std::uint32_t flags,
        std::uint32_t resource_arena_size) override;
    [[nodiscard]] std::uint32_t AttachPlugins() override;
    [[nodiscard]] std::uint32_t BindInputCallback(
        std::uint32_t selector, std::uint32_t callback_address) override;
    [[nodiscard]] std::uint32_t OpenEngine(std::uint32_t window) override;
    void ShutdownEngine() override;
    void CloseCharset();
    void CloseFileSystemManager();
    [[nodiscard]] std::uint32_t SelectDevice() override;
    void SetRefreshRate(std::uint32_t rate) override;
    void EnableDialogBoxMode(std::uint32_t enabled) override;
    [[nodiscard]] std::uint32_t StartEngine() override;
    void CloseEngine() override;
    void StopEngine() override;
    [[nodiscard]] std::uint32_t GetD3D9Caps() override;
    [[nodiscard]] std::uint32_t GetCurrentD3DDevice() override;
    [[nodiscard]] std::uint32_t InitializeImageHandlers() override;
    [[nodiscard]] std::uint32_t OpenCharset() override;
    [[nodiscard]] std::uint32_t OpenFileSystemManager(
        std::uint32_t sentinel) override;
    [[nodiscard]] std::uint32_t InitializeVirtualFileSystem() override;
    [[nodiscard]] std::uint32_t InitializeDirectInput(
        std::uint32_t instance, std::uint32_t window,
        std::uint32_t flags) override;
    void SetInputParameters(std::uint32_t first, std::uint32_t second,
                            std::uint32_t width, std::uint32_t height) override;
    void SetInputScalar(std::uint32_t float_bits) override;
    [[nodiscard]] std::uint32_t InitializeLogger() override;
    void AppendLog(std::uint32_t logger, std::string_view format,
                   std::string_view source_file,
                   std::uint32_t source_line) override;

    [[nodiscard]] const NativeRenderWareDialogModeResult& DialogModeResult()
        const { return dialog_mode_.Result(); }
    [[nodiscard]] InputManager& Input() { return input_manager_; }
    [[nodiscard]] const InputManager& Input() const { return input_manager_; }

private:
    IGameSystemsRuntime& services_;
    GameConfigWords& config_;
    GameRenderState& state_;
    IVideoDeviceRuntime& video_runtime_;
    IVirtualFileSystemRuntime& file_system_runtime_;
    RwImageCallBackRead tga_reader_;
    const RwMemoryFunctions* memory_functions_;
    std::function<void()> before_file_system_close_;
    NativeRenderWareDialogMode dialog_mode_;
    InputManager input_manager_;
    Win32InputRuntime input_runtime_;
    std::unique_ptr<NativeImageHandlerRuntime> image_handlers_;
    bool initialized_ = false;
    bool opened_ = false;
    bool started_ = false;
    bool dialog_enabled_ = false;
    bool charset_open_ = false;
    bool fs_manager_open_ = false;
};

}  // namespace pal4
