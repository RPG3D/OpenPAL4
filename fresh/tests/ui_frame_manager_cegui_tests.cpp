#include "pal4/ui_frame_manager_cegui.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::size_t Position(const std::vector<std::string>& events,
                     const std::string& event) {
    const auto found = std::find(events.begin(), events.end(), event);
    Check(found != events.end(), "missing expected event");
    return static_cast<std::size_t>(found - events.begin());
}

struct Runtime final : pal4::IUiFrameManagerCeguiRuntime {
    std::uint32_t width = 1280u;
    std::uint32_t height = 800u;
    std::uint32_t next_memory = 0x10000u;
    std::uint32_t failed_size = 0u;
    std::uint32_t renderer_bytes = 0u;
    pal4::CeguiRendererVariant renderer_variant =
        pal4::CeguiRendererVariant::Base800;
    std::uint32_t system_result = 0x4400u;
    std::uint32_t system_renderer = 0u;
    std::uint32_t system_calls = 0u;
    std::uint32_t next_window = 0x7000u;
    std::uint32_t next_image = 0x8000u;
    std::vector<std::uint32_t> key_handlers;
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint32_t> words{{0x519cu, 0x9000u}};
    std::vector<std::pair<pal4::CeguiRichTextTarget,
                          std::u32string>> rich_text;

    void ProcessPkScripts() override { events.emplace_back("pk-process"); }
    void Log(const char* file, std::uint32_t line,
             const char* format) override {
        Check(std::string(file) ==
                  "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\UiFrame\\uiFrameManager.cpp",
              "logger source file");
        const std::string text(format);
        if (line == 195u)
            Check(text == "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager initializing begin...\n\n\n",
                  "begin log format");
        else if (line == 274u)
            Check(text == "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager create Cegui helper OK\n\n\n",
                  "helper log format");
        else if (line == 335u)
            Check(text == "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager initializing OK\n\n\n",
                  "completion log format");
        else
            Check(false, "unexpected log line");
        events.emplace_back("log:" + std::to_string(line));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        events.emplace_back("allocate:" + std::to_string(size));
        if (size == failed_size) return 0u;
        const auto result = next_memory;
        next_memory += 0x1000u;
        if (size == 0xcu) key_handlers.push_back(result);
        if (size >= 0x110u && size <= 0x12cu)
            renderer_bytes = size;
        return result;
    }
    std::uint32_t ConstructExitHandler(
        std::uint32_t memory, std::uint32_t callback,
        std::uint32_t argument, std::uint32_t manager) override {
        Check(memory != 0u && callback == 0x4bfe10u &&
                  argument == 0u && manager == 0x5000u,
              "exit callback construction");
        events.emplace_back("exit-construct");
        return memory + 4u;
    }
    void InsertEvent(std::uint32_t tree, const char* key,
                     std::uint32_t handler) override {
        Check(tree == 0x5104u && std::string(key) == "ui_exit_game",
              "event tree and key");
        Check(handler == 0u || handler == 0x10004u,
              "exit callback handle");
        events.emplace_back("event-insert");
    }
    void ReleaseHandler(std::uint32_t handler) override {
        Check(handler == 0x10004u, "exit handler release");
        events.emplace_back("exit-release");
    }
    std::uint32_t ScreenWidth() override { return width; }
    std::uint32_t ScreenHeight() override { return height; }
    std::uint32_t ConstructRenderer(
        std::uint32_t memory,
        pal4::CeguiRendererVariant variant) override {
        Check(memory != 0u, "renderer allocation");
        renderer_variant = variant;
        events.emplace_back("renderer");
        return memory + 4u;
    }
    std::uint32_t InitializeSystem(
        std::uint32_t memory, std::uint32_t renderer,
        std::uint32_t resource_provider) override {
        Check(memory != 0u && resource_provider == 0u,
              "CEGUI System allocation and provider");
        system_renderer = renderer;
        ++system_calls;
        events.emplace_back("system");
        return system_result;
    }
    void SetUvAlignment(bool enabled) override {
        Check(enabled, "UV alignment argument");
        events.emplace_back("uv");
    }
    void InitializeUiFrameManager(std::uint32_t manager) override {
        Check(manager == 0x5000u, "UIFrameManager owner");
        events.emplace_back("ui-manager");
    }
    void SetDefaultFont(const char* name) override {
        Check(std::string(name) == "system", "default font");
        events.emplace_back("font");
    }
    void SetMouseCursorByte(std::uint32_t offset,
                            std::uint8_t value) override {
        Check(offset == 44u && value == 0u,
              "mouse cursor field");
        events.emplace_back("mouse-flag");
    }
    void InitializeCursors(std::uint32_t tree) override {
        Check(tree == 0x5114u, "cursor manager offset");
        events.emplace_back("cursors");
    }
    std::uint32_t CreateWindow(std::string_view type,
                               std::string_view name) override {
        if (next_window == 0x7000u)
            Check(type == "DefaultWindow" && name == "desktop",
                  "desktop type and name");
        else
            Check(type == "OLStaticImage" && name.empty(),
                  "static image type and empty name");
        events.emplace_back(next_window == 0x7000u
                                ? "desktop" : "static-window");
        const auto result = next_window;
        next_window += 0x100u;
        return result;
    }
    void SetWindowSize(std::uint32_t window,
                       std::uint32_t metrics_mode,
                       float requested_width,
                       float requested_height) override {
        Check(window == 0x7000u && metrics_mode == 1u &&
                  requested_width == 800.0f &&
                  requested_height == 600.0f,
              "desktop dimensions and metrics mode");
        events.emplace_back("size");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x519cu, "camera collection field");
        return words.at(address);
    }
    std::uint32_t SubscribeKey(std::uint32_t window,
                               bool key_down,
                               std::uint32_t handler) override {
        Check(window == 0x7000u, "keyboard subscription window");
        if (failed_size == 0xcu)
            Check(handler == 0u, "null keyboard handler");
        else
            Check(handler != 0u &&
                      words.at(handler) == 0x8443c4u &&
                      words.at(handler + 4u) ==
                          (key_down ? 0x4aedd0u : 0x4aeff0u) &&
                      words.at(handler + 8u) == 0x5134u,
                  "keyboard handler layout");
        events.emplace_back(key_down ? "key-down" : "key-up");
        return key_down ? 0x6000u : 0x6100u;
    }
    void ReleaseSubscription(
        std::uint32_t subscription) override {
        Check(subscription == 0x6000u || subscription == 0x6100u,
              "subscription release handle");
        events.emplace_back("subscription-release");
    }
    void SetGuiSheet(std::uint32_t window) override {
        Check(window == 0x7000u, "GUI sheet window");
        events.emplace_back("gui-sheet");
    }
    void CenterMouse(std::uint32_t window) override {
        Check(window == 0x7000u, "center mouse window");
        events.emplace_back("center-mouse");
    }
    std::string_view StaticImageWidgetType() override {
        events.emplace_back("widget-type");
        return "OLStaticImage";
    }
    std::uint32_t GetImage(const char* imageset,
                           const char* image) override {
        Check(std::string(imageset) == "OiramLook" &&
                  std::string(image) == "MultiLineEditboxBackdrop",
              "static image resource");
        events.emplace_back("get-image");
        return next_image++;
    }
    void SetStaticImage(std::uint32_t window,
                        std::uint32_t image) override {
        Check((window == 0x7100u || window == 0x7200u) &&
                  image >= 0x8000u && image < 0x8002u,
              "static image destination");
        events.emplace_back("set-image");
    }
    void CleanupPkScripts() override {
        events.emplace_back("pk-cleanup");
    }
    void InitializeNumericSpinnerPool() override {
        events.emplace_back("spinner-pool");
    }
    std::uint32_t ActiveCamera(
        std::uint32_t collection) override {
        Check(collection == 0x9000u, "camera collection");
        events.emplace_back("active-camera");
        return 0xa000u;
    }
    void EnableEffects(std::uint32_t manager,
                       std::uint32_t camera,
                       std::uint32_t mode) override {
        Check(manager == 0x5000u && camera == 0xa000u && mode == 0u,
              "effects enable arguments");
        events.emplace_back("effects");
    }
    std::string GameDbText(std::uint32_t id) override {
        events.emplace_back("db:" + std::to_string(id));
        if (id == 310u) return "A\xE4\xB8\xAD";
        if (id == 311u) return "tail";
        if (id == 309u) return "\xF0\x9F\x98\x80";
        throw std::runtime_error("unexpected GameDB id");
    }
    std::string ConvertToUtf8(
        std::string_view source) override {
        events.emplace_back("utf8");
        return std::string(source);
    }
    std::uint32_t SetRichText(
        pal4::CeguiRichTextTarget target,
        std::u32string_view value) override {
        rich_text.emplace_back(target, std::u32string(value));
        events.emplace_back("rich-text");
        return target == pal4::CeguiRichTextTarget::WrapDelimiters
                   ? 0x1234u : 0x7777u;
    }
};

void CheckRenderer(const std::uint32_t width,
                   const std::uint32_t height,
                   const pal4::CeguiRendererVariant variant,
                   const std::uint32_t bytes,
                   const bool align_uv) {
    Runtime runtime;
    runtime.width = width;
    runtime.height = height;
    Check(pal4::UiFrameManagerCegui::Initialize(
              0x5000u, runtime) == 0x1234u,
          "returns WrapDelimiters buffer pointer");
    Check(runtime.renderer_variant == variant &&
              runtime.renderer_bytes == bytes &&
              runtime.system_calls == 1u &&
              runtime.system_renderer != 0u &&
              runtime.words.at(0x5198u) == 0x4400u,
          "renderer and system allocation plan");
    Check((std::find(runtime.events.begin(), runtime.events.end(),
                     "uv") != runtime.events.end()) == align_uv,
          "UV alignment resolution gate");
    Check(runtime.key_handlers.size() == 2u &&
              runtime.key_handlers[0] != runtime.key_handlers[1],
          "distinct keyboard callback allocations");
    Check(Position(runtime.events, "pk-process") <
              Position(runtime.events, "log:195") &&
              Position(runtime.events, "event-insert") <
              Position(runtime.events, "system") &&
              Position(runtime.events, "system") <
              Position(runtime.events, "log:274") &&
              Position(runtime.events, "ui-manager") <
              Position(runtime.events, "font") &&
              Position(runtime.events, "cursors") <
              Position(runtime.events, "desktop") &&
              Position(runtime.events, "key-down") <
              Position(runtime.events, "key-up") &&
              Position(runtime.events, "gui-sheet") <
              Position(runtime.events, "center-mouse") &&
              Position(runtime.events, "pk-cleanup") <
              Position(runtime.events, "spinner-pool") &&
              Position(runtime.events, "log:335") <
              Position(runtime.events, "active-camera") &&
              Position(runtime.events, "effects") <
              Position(runtime.events, "db:310"),
          "initialization stage order");
    const auto static_windows = std::count(
        runtime.events.begin(), runtime.events.end(),
        "static-window");
    Check(static_windows == (width == 1280u ? 2 : 0),
          "1280-only static windows");
    if (width == 1280u) {
        Check(runtime.words.at(0x51d4u) == 0x7100u &&
                  runtime.words.at(0x51d8u) == 0x7200u &&
                  std::count(runtime.events.begin(),
                             runtime.events.end(), "get-image") == 2 &&
                  Position(runtime.events, "static-window") <
                      Position(runtime.events, "get-image"),
              "static image fields and resource order");
    }
    Check(runtime.rich_text.size() == 3u &&
              runtime.rich_text[0].first ==
                  pal4::CeguiRichTextTarget::Special &&
              runtime.rich_text[0].second ==
                  std::u32string{U'A', U'\u4e2d'} &&
              runtime.rich_text[1].first ==
                  pal4::CeguiRichTextTarget::TailForbid &&
              runtime.rich_text[1].second == U"tail" &&
              runtime.rich_text[2].first ==
                  pal4::CeguiRichTextTarget::WrapDelimiters &&
              runtime.rich_text[2].second ==
                  std::u32string{U'\U0001f600'},
          "GameDB text IDs and UTF-8 decoding");
}

void CheckAllocationFailures() {
    Runtime exit_failure;
    exit_failure.width = 1024u;
    exit_failure.failed_size = 0x18u;
    Check(pal4::UiFrameManagerCegui::Initialize(
              0x5000u, exit_failure) == 0x1234u &&
              std::find(exit_failure.events.begin(),
                        exit_failure.events.end(),
                        "exit-construct") == exit_failure.events.end(),
          "null exit callback still allows event insertion");

    Runtime system_failure;
    system_failure.width = 1024u;
    system_failure.failed_size = 0x20u;
    Check(pal4::UiFrameManagerCegui::Initialize(
              0x5000u, system_failure) == 0x1234u &&
              system_failure.system_calls == 0u &&
              system_failure.words.at(0x5198u) == 0u &&
              Position(system_failure.events, "uv") <
                  Position(system_failure.events, "log:274"),
          "null CEGUI System allocation keeps later calls");

    Runtime renderer_failure;
    renderer_failure.width = 1024u;
    renderer_failure.failed_size = 0x118u;
    Check(pal4::UiFrameManagerCegui::Initialize(
              0x5000u, renderer_failure) == 0x1234u &&
              renderer_failure.system_calls == 1u &&
              renderer_failure.system_renderer == 0u,
          "null renderer passed to system constructor");

    Runtime key_failure;
    key_failure.width = 1024u;
    key_failure.failed_size = 0xcu;
    Check(pal4::UiFrameManagerCegui::Initialize(
              0x5000u, key_failure) == 0x1234u &&
              key_failure.key_handlers.empty() &&
              Position(key_failure.events, "key-down") <
                  Position(key_failure.events, "key-up"),
          "null keyboard handlers still subscribe");
}

}  // namespace

int main() {
    try {
        CheckRenderer(1280u, 800u,
                      pal4::CeguiRendererVariant::Renderer5,
                      0x12cu, true);
        CheckRenderer(1280u, 960u,
                      pal4::CeguiRendererVariant::Renderer4,
                      0x120u, true);
        CheckRenderer(1280u, 720u,
                      pal4::CeguiRendererVariant::Renderer2,
                      0x118u, true);
        CheckRenderer(1152u, 864u,
                      pal4::CeguiRendererVariant::Renderer3,
                      0x120u, true);
        CheckRenderer(1024u, 768u,
                      pal4::CeguiRendererVariant::Renderer2,
                      0x118u, true);
        CheckRenderer(800u, 600u,
                      pal4::CeguiRendererVariant::Base800,
                      0x110u, false);
        CheckRenderer(1680u, 1050u,
                      pal4::CeguiRendererVariant::Renderer2,
                      0x118u, true);
        CheckRenderer(1920u, 1080u,
                      pal4::CeguiRendererVariant::Renderer2,
                      0x118u, true);
        CheckRenderer(1600u, 900u,
                      pal4::CeguiRendererVariant::Renderer2,
                      0x118u, true);
        CheckAllocationFailures();
        std::cout << "ui_frame_manager_cegui_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
