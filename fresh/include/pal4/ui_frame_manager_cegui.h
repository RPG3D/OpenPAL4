#pragma once

#include "pal4/cegui_renderer_profile.h"
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

enum class CeguiRichTextTarget : std::uint8_t {
    Special,
    TailForbid,
    WrapDelimiters,
};

class IUiFrameManagerCeguiRuntime {
public:
    virtual ~IUiFrameManagerCeguiRuntime() = default;
    virtual void ProcessPkScripts() = 0;
    virtual void Log(const char* file, std::uint32_t line,
                     const char* format) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructExitHandler(
        std::uint32_t memory, std::uint32_t callback,
        std::uint32_t argument, std::uint32_t manager) = 0;
    virtual void InsertEvent(std::uint32_t tree, const char* key,
                             std::uint32_t handler) = 0;
    virtual void ReleaseHandler(std::uint32_t handler) = 0;
    [[nodiscard]] virtual std::uint32_t ScreenWidth() = 0;
    [[nodiscard]] virtual std::uint32_t ScreenHeight() = 0;
    [[nodiscard]] virtual std::uint32_t ConstructRenderer(
        std::uint32_t memory, CeguiRendererVariant variant) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeSystem(
        std::uint32_t memory, std::uint32_t renderer,
        std::uint32_t resource_provider) = 0;
    virtual void SetUvAlignment(bool enabled) = 0;
    virtual void InitializeUiFrameManager(std::uint32_t manager) = 0;
    virtual void SetDefaultFont(const char* name) = 0;
    virtual void SetMouseCursorByte(std::uint32_t offset,
                                    std::uint8_t value) = 0;
    virtual void InitializeCursors(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t CreateWindow(
        std::string_view type, std::string_view name) = 0;
    virtual void SetWindowSize(std::uint32_t window,
                               std::uint32_t metrics_mode,
                               float width, float height) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeKey(
        std::uint32_t window, bool key_down,
        std::uint32_t handler) = 0;
    virtual void ReleaseSubscription(std::uint32_t subscription) = 0;
    virtual void SetGuiSheet(std::uint32_t window) = 0;
    virtual void CenterMouse(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::string_view StaticImageWidgetType() = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        const char* imageset, const char* image) = 0;
    virtual void SetStaticImage(std::uint32_t window,
                                std::uint32_t image) = 0;
    virtual void CleanupPkScripts() = 0;
    virtual void InitializeNumericSpinnerPool() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCamera(
        std::uint32_t camera_collection) = 0;
    virtual void EnableEffects(std::uint32_t manager,
                               std::uint32_t camera,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::string GameDbText(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view source) = 0;
    [[nodiscard]] virtual std::uint32_t SetRichText(
        CeguiRichTextTarget target,
        std::u32string_view value) = 0;
};

class ICeguiRendererDestructorRuntime {
public:
    virtual ~ICeguiRendererDestructorRuntime() = default;
    virtual void DestructRenderer(std::uint32_t renderer) = 0;
};

class UiFrameManagerCegui final {
public:
    // IDA 0x4bc360. Returns the WrapDelimiters CEGUI string data pointer.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t manager,
        IUiFrameManagerCeguiRuntime& runtime);

    // IDA 0x4bd310: direct thunk to the CEGUI Renderer destructor.
    static void RendererDestructorThunk(
        std::uint32_t renderer,
        ICeguiRendererDestructorRuntime& runtime);
};

}  // namespace pal4
