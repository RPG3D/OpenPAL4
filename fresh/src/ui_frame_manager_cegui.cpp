#include "pal4/ui_frame_manager_cegui.h"

#include "pal4/preview_text_codec.h"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace pal4 {
namespace {

constexpr char kSourceFile[] =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\UiFrame\\uiFrameManager.cpp";
constexpr char kBeginLog[] =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager initializing begin...\n\n\n";
constexpr char kHelperLog[] =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager create Cegui helper OK\n\n\n";
constexpr char kDoneLog[] =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager initializing OK\n\n\n";

void SubscribeKeyboard(const std::uint32_t manager,
                       const std::uint32_t window,
                       const bool key_down,
                       const std::uint32_t callback,
                       IUiFrameManagerCeguiRuntime& runtime) {
    const auto handler = runtime.Allocate(0xcu);
    if (handler != 0u) {
        runtime.WriteWord(handler, 0x8443c4u);
        runtime.WriteWord(handler + 4u, callback);
        runtime.WriteWord(handler + 8u, manager + 0x134u);
    }
    const auto subscription = runtime.SubscribeKey(
        window, key_down, handler);
    if (subscription != 0u)
        runtime.ReleaseSubscription(subscription);
}

void InitializeRichText(const std::uint32_t id,
                        const CeguiRichTextTarget target,
                        IUiFrameManagerCeguiRuntime& runtime) {
    const auto raw = runtime.GameDbText(id);
    const auto utf8 = runtime.ConvertToUtf8(raw);
    (void)runtime.SetRichText(target,
                              PreviewTextCodec::DecodeUtf8(utf8));
}

}  // namespace

std::uint32_t UiFrameManagerCegui::Initialize(
    const std::uint32_t manager,
    IUiFrameManagerCeguiRuntime& runtime) {
    runtime.ProcessPkScripts();
    runtime.Log(kSourceFile, 195u, kBeginLog);
    const auto handler_memory = runtime.Allocate(0x18u);
    const auto exit_handler = handler_memory == 0u ? 0u :
        runtime.ConstructExitHandler(handler_memory, 0x4bfe10u,
                                     0u, manager);
    runtime.InsertEvent(manager + 0x104u, "ui_exit_game",
                        exit_handler);
    if (exit_handler != 0u) runtime.ReleaseHandler(exit_handler);

    const auto width = runtime.ScreenWidth();
    const auto height = runtime.ScreenHeight();
    const auto renderer_plan = SelectCeguiRendererPlan(width, height);
    const auto system_memory = runtime.Allocate(0x20u);
    std::uint32_t system = 0u;
    if (system_memory != 0u) {
        const auto renderer_memory = runtime.Allocate(renderer_plan.bytes);
        const auto renderer = renderer_memory == 0u ? 0u :
            runtime.ConstructRenderer(renderer_memory,
                                      renderer_plan.variant);
        system = runtime.InitializeSystem(system_memory, renderer, 0u);
    }
    runtime.WriteWord(manager + 0x198u, system);
    if (renderer_plan.align_uv) runtime.SetUvAlignment(true);

    runtime.Log(kSourceFile, 274u, kHelperLog);
    runtime.InitializeUiFrameManager(manager);
    runtime.SetDefaultFont("system");
    runtime.SetMouseCursorByte(44u, 0u);
    runtime.InitializeCursors(manager + 0x114u);

    const auto desktop = runtime.CreateWindow("DefaultWindow", "desktop");
    runtime.SetWindowSize(desktop, 1u, 800.0f, 600.0f);
    SubscribeKeyboard(manager, desktop, true, 0x4aedd0u, runtime);
    SubscribeKeyboard(manager, desktop, false, 0x4aeff0u, runtime);
    runtime.SetGuiSheet(desktop);
    runtime.CenterMouse(desktop);

    if (width == 1280u) {
        const auto type = runtime.StaticImageWidgetType();
        const auto first = runtime.CreateWindow(type, "");
        runtime.WriteWord(manager + 0x1d4u, first);
        const auto second = runtime.CreateWindow(type, "");
        runtime.WriteWord(manager + 0x1d8u, second);
        runtime.SetStaticImage(first, runtime.GetImage(
            "OiramLook", "MultiLineEditboxBackdrop"));
        runtime.SetStaticImage(second, runtime.GetImage(
            "OiramLook", "MultiLineEditboxBackdrop"));
    }

    runtime.CleanupPkScripts();
    runtime.InitializeNumericSpinnerPool();
    runtime.Log(kSourceFile, 335u, kDoneLog);
    runtime.EnableEffects(
        manager,
        runtime.ActiveCamera(runtime.ReadWord(manager + 0x19cu)),
        0u);
    InitializeRichText(310u, CeguiRichTextTarget::Special, runtime);
    InitializeRichText(311u, CeguiRichTextTarget::TailForbid, runtime);
    const auto raw = runtime.GameDbText(309u);
    const auto utf8 = runtime.ConvertToUtf8(raw);
    return runtime.SetRichText(
        CeguiRichTextTarget::WrapDelimiters,
        PreviewTextCodec::DecodeUtf8(utf8));
}

}  // namespace pal4
