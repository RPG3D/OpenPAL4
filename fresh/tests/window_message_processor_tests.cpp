#include "pal4/window_message_processor.h"

#if defined(_WIN32) && !defined(_WIN64)
#include "pal4/win32_game_window_runtime.h"
#include "pal4/win32_window_message_binding.h"
#include "pal4/win32_window_message_runtime.h"
#include <windows.h>
#include <shellapi.h>
#include <shlobj_core.h>
#undef DefWindowProc
#endif

#include <bit>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(float value) { return std::bit_cast<std::uint32_t>(value); }

class Runtime final : public pal4::IWindowMessageRuntime {
public:
    std::vector<std::string> calls;
    std::vector<pal4::DisplayModeRecord> modes;
    std::vector<std::string> files;
    std::vector<std::uint32_t> file_indexes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> keyboard_events;
    std::vector<std::uint32_t> mouse_events;
    const std::uint32_t* keyboard_config = nullptr;
    const std::uint32_t* mouse_config = nullptr;
    std::pair<bool, bool> ready{true, true};
    std::uint32_t created_callback = 0x900u;
    std::uint32_t ui_count = 0u;
    std::uint32_t default_count = 0u;
    std::uint32_t music_handle = 0u;
    std::uint32_t camera = 0u;
    std::int32_t width = 0;
    std::int32_t height = 0;
    bool console = false;
    bool audio_active = false;
    bool music_enabled = true;
    std::string music_path;
    bool native = false;
    std::uint32_t observed_window = 0u;

    void AcceptDroppedFiles(std::uint32_t window) override {
        if (native) {
            Check(window != 0u, "native drop acceptance window");
            observed_window = window;
        } else {
            Check(window == 0x111u, "drop acceptance window");
        }
        calls.emplace_back("accept");
    }
    void EnumCurrentDisplayMode(pal4::DisplayModeRecord& mode) override {
        calls.emplace_back("enum-mode");
        mode.WriteDword(108u, 1024u);
    }
    void ChangeDisplayMode(const pal4::DisplayModeRecord& mode,
                           std::uint32_t flags) override {
        Check(flags == 0x40000009u, "display mode flags");
        calls.emplace_back("display");
        modes.push_back(mode);
    }
    void ReleaseCursorClip() override { calls.emplace_back("unclip"); }
    void ClipCursorToWindow(std::uint32_t window) override {
        Check(window == 0x111u, "clip window");
        calls.emplace_back("clip");
    }
    void SetInputMode(std::uint32_t keyboard, std::uint32_t mouse,
                      std::uint32_t flags) override {
        Check(keyboard == mouse && keyboard <= 1u && flags == 0u,
              "input mode arguments");
        calls.emplace_back("input-mode");
    }
    void InitializeInputState() override { calls.emplace_back("input-init"); }
    std::pair<bool, bool> InputDevicesReady() override {
        calls.emplace_back("input-ready");
        return ready;
    }
    void ShowWindow(std::uint32_t window, std::uint32_t command) override {
        Check(window == 0x111u, "show window");
        calls.emplace_back("show-" + std::to_string(command));
    }
    void SetForegroundWindow(std::uint32_t window) override {
        Check(window == 0x111u, "foreground window");
        calls.emplace_back("foreground");
    }
    void SetFocus(std::uint32_t window) override {
        Check(window == 0x111u, "focused window");
        calls.emplace_back("focus");
    }
    void SetAudioActive(bool active) override {
        calls.emplace_back(active ? "audio-on" : "audio-off");
    }
    void Sleep(std::uint32_t milliseconds) override {
        Check(milliseconds == 500u, "activation sleep");
        calls.emplace_back("sleep");
    }
    std::uint32_t TimeGetTime() override {
        calls.emplace_back("tick");
        return 0x2468u;
    }
    void SetCapture(std::uint32_t window) override {
        Check(window == 0x111u, "capture window");
        calls.emplace_back("capture");
    }
    void ReleaseCapture() override { calls.emplace_back("release-capture"); }
    void HandleKeyboardInput(
        std::uint32_t type, pal4::KeyboardInputArgument argument) override {
        std::uint32_t value = 0u;
        if (type == 0x18u) {
            Check(std::holds_alternative<std::uint32_t>(argument),
                  "character argument is a value");
            value = std::get<std::uint32_t>(argument);
        } else {
            Check(std::holds_alternative<const std::uint32_t*>(argument) &&
                      std::get<const std::uint32_t*>(argument) ==
                          keyboard_config,
                  "key argument is a config pointer");
            value = *keyboard_config;
        }
        keyboard_events.emplace_back(type, value);
        calls.emplace_back("keyboard");
    }
    void HandleMouseInput(std::uint32_t type,
                          const std::uint32_t* data) override {
        Check(data == mouse_config, "mouse config data source");
        mouse_events.push_back(type);
        calls.emplace_back("mouse");
    }
    void SetUICursorFromTable(std::uint32_t table) override {
        Check(table == 0x8b1c70u, "UI cursor table");
        calls.emplace_back("cursor");
    }
    bool CurrentAudioActive() override {
        calls.emplace_back("audio-check");
        return audio_active;
    }
    void ReplayCurrentSound() override { calls.emplace_back("audio-replay"); }
    std::string ComposeMusicBasePath() override {
        calls.emplace_back("music-base");
        return "Music/";
    }
    bool MusicEnabled() override {
        calls.emplace_back("music-enabled");
        return music_enabled;
    }
    void SetMusicEnabled(bool value) override {
        calls.emplace_back("music-set");
        music_enabled = value;
    }
    std::uint32_t PlayMusic(std::string_view path,
                            std::uint32_t flags) override {
        Check(flags == 1u, "music flags");
        calls.emplace_back("music-play");
        music_path = path;
        return 0xbabau;
    }
    void StoreMusicHandle(std::uint32_t value) override {
        calls.emplace_back("music-store");
        music_handle = value;
    }
    pal4::ClientRect ReadSizingRect(std::uint32_t address) override {
        Check(address == 0xfaceu, "sizing rect address");
        calls.emplace_back("sizing-rect");
        return {5, 10, 805, 610};
    }
    void SetWindowPos(std::uint32_t window, std::int32_t new_width,
                      std::int32_t new_height, std::uint32_t flags) override {
        Check(window == 0x111u && flags == 2u, "window position flags");
        calls.emplace_back("window-pos");
        width = new_width;
        height = new_height;
    }
    void GetRenderSystem() override { calls.emplace_back("render-system"); }
    std::uint32_t GetActiveCamera() override {
        calls.emplace_back("active-camera");
        return 0x7777u;
    }
    void ResizeRenderScreen(std::int32_t new_width,
                            std::int32_t new_height,
                            std::uint32_t active_camera) override {
        Check(new_width == width && new_height == height,
              "camera resize dimensions");
        calls.emplace_back("camera-resize");
        camera = active_camera;
    }
    std::uint32_t GetFilesystemCallback() override {
        calls.emplace_back("filesystem-get");
        return 0x777u;
    }
    std::uint32_t DroppedFileCount(std::uint32_t drop) override {
        Check(drop == 0x444u, "drop count handle");
        calls.emplace_back("drop-count");
        return static_cast<std::uint32_t>(files.size());
    }
    std::string DroppedFile(std::uint32_t drop,
                            std::uint32_t index) override {
        Check(drop == 0x444u, "drop file handle");
        calls.emplace_back("drop-file");
        file_indexes.push_back(index);
        return files.at(index);
    }
    std::uint32_t CreateUncdFilesystemCallback(
        std::uint32_t id, std::string_view label) override {
        Check(id == 20u && label == "UNCD", "UNCD callback fields");
        calls.emplace_back("filesystem-create");
        return created_callback;
    }
    void RegisterFilesystemCallback(std::uint32_t callback) override {
        Check(callback == created_callback, "registered callback");
        calls.emplace_back("filesystem-register");
    }
    std::uint32_t FindFilesystemCallback(std::string_view label) override {
        Check(label == "UNCD", "find UNCD label");
        calls.emplace_back("filesystem-find");
        return 0x901u;
    }
    void SetFilesystemCallback(std::uint32_t callback) override {
        calls.emplace_back("filesystem-set-" + std::to_string(callback));
    }
    void FinishDrop(std::uint32_t drop) override {
        Check(drop == 0x444u, "finish drop handle");
        calls.emplace_back("drop-finish");
    }
    bool ConsoleActive() override {
        calls.emplace_back("console");
        return console;
    }
    void HandleUIMessage(std::uint32_t window, std::uint32_t message,
                         std::uint32_t, std::uint32_t) override {
        Check(window == 0x111u && message != 0u, "UI forwarding");
        calls.emplace_back("ui");
        ++ui_count;
    }
    std::uint32_t DefWindowProc(std::uint32_t window,
                                std::uint32_t message,
                                std::uint32_t wparam,
                                std::uint32_t lparam) override {
        Check((native ? window != 0u : window == 0x111u) && message != 0u,
              "default forwarding");
        calls.emplace_back("default");
        ++default_count;
#if defined(_WIN32) && !defined(_WIN64)
        if (native) {
            return static_cast<std::uint32_t>(::DefWindowProcA(
                reinterpret_cast<HWND>(static_cast<std::uintptr_t>(window)),
                message, wparam, lparam));
        }
#endif
        return 0x77u;
    }
};

struct Fixture {
    pal4::GameRenderState game{0x800000u};
    pal4::WindowMessageConfig config{};
    pal4::DisplayModeRecord mode;
    std::uint32_t resize_marker = 0x55u;
    pal4::WindowMessageProcessor processor{game, config, mode,
                                             resize_marker};
    Runtime runtime;
    Fixture() {
        config[0] = 800u;
        config[1] = 600u;
        runtime.keyboard_config = config.data() + 25u;
        runtime.mouse_config = config.data() + 12u;
    }
    std::uint32_t Send(std::uint32_t message, std::uint32_t wparam = 0u,
                       std::uint32_t lparam = 0u) {
        return processor.Process(0x111u, message, wparam, lparam, runtime);
    }
};

void CheckCalls(const Runtime& runtime,
                const std::vector<std::string>& expected,
                const char* message) {
    Check(runtime.calls == expected, message);
}

void TestScanAndFallback() {
    using pal4::WindowMessageProcessor;
    Check(WindowMessageProcessor::DecodeKeyboardScanCode(99u, 0x001e0000u) ==
              0x1eu &&
              WindowMessageProcessor::DecodeKeyboardScanCode(
                  0u, 0x011e0000u) == 0x9eu &&
              WindowMessageProcessor::DecodeKeyboardScanCode(
                  0u, 0x00800000u) == 0x80u,
          "scan code and extended bit");
    Check(WindowMessageProcessor::MapScanCode(0x2au) == 172u &&
              WindowMessageProcessor::MapScanCode(0x2fu) == 118u &&
              WindowMessageProcessor::MapScanCode(0x9cu) == 166u &&
              WindowMessageProcessor::MapScanCode(0xffu) == 255u,
          "IDA scan-code table samples");
    std::uint32_t scan_map_hash = 2166136261u;
    for (std::uint32_t scan = 0; scan < 256u; ++scan) {
        scan_map_hash =
            (scan_map_hash ^ WindowMessageProcessor::MapScanCode(
                                 static_cast<std::uint8_t>(scan))) *
            16777619u;
    }
    Check(scan_map_hash == 0x0ccb66a9u,
          "complete IDA scan-code table");
    Fixture f;
    f.game.Write(0x28u, 1u);
    Check(f.Send(0x999u) == 0x77u &&
              f.processor.HandledFlag() == 0u && f.runtime.ui_count == 1u &&
              f.runtime.default_count == 1u,
          "unknown message reaches UI and default procedure");
    CheckCalls(f.runtime, {"console", "ui", "default"},
               "unknown message call order");
    f.runtime.calls.clear();
    f.runtime.console = true;
    Check(f.Send(0x113u) == 0u && f.processor.HandledFlag() == 1u,
          "timer is handled");
    CheckCalls(f.runtime, {"console"}, "console suppresses UI forwarding");
}

void TestWindowAndActivation() {
    Fixture f;
    Check(f.Send(1u) == 0u && f.mode.ReadDword(108u) == 1024u,
          "create accepts files and saves mode");
    CheckCalls(f.runtime, {"accept", "enum-mode", "console"},
               "create call order");
    f.runtime.calls.clear();
    f.game.Write(8u, 1u);
    f.config[3] = 0x12345600u;
    Check(f.Send(0x10u) == 0u && f.config[3] == 0x12345601u &&
              f.runtime.modes.back().ReadDword(108u) == 1024u,
          "close restores mode and sets only config byte");
    CheckCalls(f.runtime, {"display", "unclip", "console"},
               "close call order");
    f.runtime.calls.clear();
    (void)f.Send(2u);
    CheckCalls(f.runtime, {"display", "unclip", "console"},
               "destroy shares close path");
    f.runtime.calls.clear();
    f.game.Write(0x548u, 1u);
    (void)f.Send(3u);
    CheckCalls(f.runtime, {"clip", "console"}, "move clips cursor");
    f.runtime.calls.clear();
    f.game.Write(0x24u, 1u);
    (void)f.Send(5u, 0u, 0x02580320u);
    Check(f.resize_marker == 0u, "nonzero size clears marker");
    f.resize_marker = 0x55u;
    (void)f.Send(5u, 0u, 0x02580000u);
    Check(f.resize_marker == 0x55u, "zero width preserves marker");
    f.runtime.calls.clear();
    (void)f.Send(0x20u);
    CheckCalls(f.runtime, {"cursor", "console"}, "UI cursor command");

    f.runtime.calls.clear();
    f.game.Write(0x28u, 1u);
    for (std::uint32_t i = 12u; i <= 31u; ++i) f.config[i] = i;
    Check(f.Send(6u, 1u) == 0u && f.game.Read(0x20u) == 1u &&
              f.game.Read(0x56cu) == 0x2468u &&
              f.game.Read(0x570u) == 0x2468u &&
              f.game.Read(0x574u) == 0x2468u &&
              f.runtime.modes.back().ReadDword(108u) == 800u &&
              f.runtime.modes.back().ReadDword(112u) == 600u &&
              f.runtime.modes.back().ReadDword(40u) == 0x180000u,
          "activation sets requested mode and clock state");
    for (std::uint32_t i = 12u; i <= 31u; ++i)
        Check(f.config[i] == 0u, "activation clears input words");
    CheckCalls(f.runtime,
               {"display", "input-mode", "input-init", "clip", "tick",
                "show-9", "show-5", "foreground", "focus", "audio-on",
                "sleep", "console", "ui"},
               "activation order and UI forwarding");
    f.runtime.calls.clear();
    Check(f.Send(6u) == 0u && f.game.Read(0x20u) == 0u &&
              f.game.Read(0x28u) == 0u &&
              f.runtime.modes.back().ReadDword(108u) == 1024u,
          "deactivation restores mode and active flags");
    CheckCalls(f.runtime,
               {"input-mode", "input-init", "unclip", "show-2", "display",
                "audio-off", "console"}, "deactivation call order");

    f.runtime.calls.clear();
    f.game.Write(0x28u, 1u);
    f.game.Write(0x20u, 1u);
    f.runtime.ready = {false, true};
    Check(f.Send(0x47u) == 0x77u &&
              f.processor.HandledFlag() == 0u,
          "input-restoration message remains unhandled");
    CheckCalls(f.runtime,
               {"input-ready", "input-mode", "input-init", "show-5",
                "foreground", "focus", "console", "ui", "default"},
               "input restoration call order");
}

void TestKeyboard() {
    Fixture f;
    f.game.Write(0x20u, 1u);
    Check(f.Send(0x100u, 0u, 0x002a0000u) == 0u &&
              f.config[25] == 0x2au && f.config[26] == 172u &&
              f.config[27] == 1u,
          "key down maps scan and sets modifier");
    CheckCalls(f.runtime, {"keyboard", "console"},
               "key down call order");
    Check(f.runtime.keyboard_events.back() ==
              std::pair<std::uint32_t, std::uint32_t>{0x16u, 0x2au},
          "key down callback type and scan pointer");
    f.runtime.calls.clear();
    f.runtime.audio_active = true;
    Check(f.Send(0x101u, 0u, 0x002f0000u) == 0u &&
              f.config[25] == 0x2fu && !f.runtime.music_enabled &&
              f.runtime.music_path == "Music/HSOff.mp3" &&
              f.runtime.music_handle == 0xbabau,
          "key up toggles music and stores new handle");
    CheckCalls(f.runtime,
               {"keyboard", "audio-check", "audio-replay",
                "music-base", "music-enabled", "music-set", "music-play",
                "music-store", "console"}, "music toggle call order");
    Check(f.runtime.keyboard_events.back() ==
              std::pair<std::uint32_t, std::uint32_t>{0x17u, 0x2fu},
          "key up callback type and scan pointer");
    f.runtime.calls.clear();
    (void)f.Send(0x101u, 0u, 0x002f0000u);
    Check(f.runtime.music_enabled &&
              f.runtime.music_path == "Music/HSOn.mp3",
          "second shortcut enables music");

    f.runtime.calls.clear();
    f.game.Write(0x20u, 0u);
    (void)f.Send(0x100u, 0u, 0x001e0000u);
    CheckCalls(f.runtime, {"console"},
               "inactive normal key down is swallowed");
    f.runtime.calls.clear();
    (void)f.Send(0x104u, 0u, 0x40000001u);
    CheckCalls(f.runtime, {"console"},
               "repeated system key down is swallowed");
    f.runtime.calls.clear();
    (void)f.Send(0x104u, 0u, 0x001e0000u);
    CheckCalls(f.runtime, {"keyboard", "console"},
               "nonrepeated system key dispatches while inactive");
    Check(f.runtime.keyboard_events.back().first == 0x16u,
          "system key down callback type");
    f.runtime.calls.clear();
    f.config[31] = 1u;
    (void)f.Send(0x105u, 0u, 0x001e0001u);
    Check(f.config[31] == 0u, "system key up clears special flag");
    CheckCalls(f.runtime, {"keyboard", "console"},
               "system key up call order");
    Check(f.runtime.keyboard_events.back().first == 0x17u,
          "system key up callback type");
    f.runtime.calls.clear();
    (void)f.Send(0x102u, 0x41u);
    CheckCalls(f.runtime, {"keyboard", "console"},
               "character input dispatches while inactive");
    Check(f.runtime.keyboard_events.back() ==
              std::pair<std::uint32_t, std::uint32_t>{0x18u, 0x41u},
          "character callback uses wParam");
}

void TestMouse() {
    Fixture f;
    f.game.Write(0x20u, 1u);
    (void)f.Send(0x200u, 0u, 0x0014000au);
    Check(f.game.Read(0x2cu) == 1u &&
              f.game.Read(0x10u) == Bits(10.0f) &&
              f.game.Read(0x14u) == Bits(20.0f),
          "first move only seeds prior coordinates");
    CheckCalls(f.runtime, {"console"}, "first move skips callback");
    f.runtime.calls.clear();
    f.config[27] = 1u;
    f.config[28] = 2u;
    (void)f.Send(0x200u, 0x13u, 0x0018000fu);
    Check(f.config[12] == Bits(15.0f) &&
              f.config[13] == Bits(24.0f) &&
              f.config[14] == Bits(5.0f) &&
              f.config[15] == Bits(4.0f) &&
              f.config[20] == 1u && f.config[21] == 2u &&
              f.config[17] == 1u && f.config[18] == 1u &&
              f.config[19] == 1u,
          "later move calculates deltas and button snapshot");
    CheckCalls(f.runtime, {"mouse", "console"}, "later move dispatches");
    Check(f.runtime.mouse_events.back() == 0x0cu, "mouse move event type");
    f.runtime.calls.clear();
    (void)f.Send(0x201u, 0u, 0x001e0014u);
    Check(f.config[17] == 1u && f.config[14] == 0u &&
              f.game.Read(0x10u) == Bits(20.0f),
          "left down updates prior coordinates");
    CheckCalls(f.runtime, {"capture", "mouse", "console"},
               "left down capture order");
    Check(f.runtime.mouse_events.back() == 5u, "left down event type");
    f.runtime.calls.clear();
    (void)f.Send(0x202u);
    Check(f.config[17] == 0u, "left up clears button");
    CheckCalls(f.runtime, {"release-capture", "mouse", "console"},
               "left up release order");
    Check(f.runtime.mouse_events.back() == 6u, "left up event type");
    f.runtime.calls.clear();
    (void)f.Send(0x203u);
    Check(f.runtime.mouse_events.back() == 0x0bu,
          "left double-click event type");
    f.runtime.calls.clear();
    (void)f.Send(0x204u);
    Check(f.config[18] == 1u, "right down button state");
    CheckCalls(f.runtime, {"capture", "mouse", "console"},
               "right down capture");
    Check(f.runtime.mouse_events.back() == 7u, "right down event type");
    f.runtime.calls.clear();
    (void)f.Send(0x205u);
    Check(f.config[18] == 0u, "right up button state");
    CheckCalls(f.runtime, {"release-capture", "mouse", "console"},
               "right up release");
    Check(f.runtime.mouse_events.back() == 8u, "right up event type");
    f.runtime.calls.clear();
    (void)f.Send(0x207u);
    Check(f.config[19] == 1u, "middle down button state");
    CheckCalls(f.runtime,
               {"release-capture", "unclip", "mouse", "console"},
               "middle down release order");
    Check(f.runtime.mouse_events.back() == 9u, "middle down event type");
    f.runtime.calls.clear();
    (void)f.Send(0x208u);
    Check(f.config[19] == 0u, "middle up button state");
    CheckCalls(f.runtime,
               {"release-capture", "unclip", "mouse", "console"},
               "middle up release order");
    Check(f.runtime.mouse_events.back() == 0x0au,
          "middle up event type");
    f.runtime.calls.clear();
    (void)f.Send(0x20au, 0x80000003u);
    Check(f.config[17] == 1u && f.config[18] == 1u &&
              f.config[16] == 0u,
          "wheel button and direction fields");
    CheckCalls(f.runtime, {"mouse", "console"}, "wheel dispatch");
    Check(f.runtime.mouse_events.back() == 0x0du, "wheel event type");
    f.runtime.calls.clear();
    f.game.Write(0x20u, 0u);
    (void)f.Send(0x203u);
    CheckCalls(f.runtime, {"console"},
               "inactive mouse message skips dispatch");
}

void TestSizingAndDrop() {
    Fixture f;
    Check(f.Send(0x214u, 0u, 0xfaceu) == 0u &&
              f.runtime.width == 800 && f.runtime.height == 600 &&
              f.runtime.camera == 0x7777u,
          "sizing forwards dimensions and camera");
    CheckCalls(f.runtime,
               {"sizing-rect", "window-pos", "render-system",
                "active-camera", "camera-resize", "console"},
               "sizing call order");
    f.runtime.calls.clear();
    f.runtime.files = {"plain.txt", "C:\\one.txt",
                       "\\\\server\\share\\two.txt"};
    (void)f.Send(0x233u, 0x444u);
    Check(f.runtime.file_indexes ==
              std::vector<std::uint32_t>{2u, 1u, 0u},
          "drop scans in reverse index order");
    CheckCalls(f.runtime,
               {"filesystem-get", "drop-count", "drop-file",
                "filesystem-create", "filesystem-register",
                "filesystem-set-2304", "drop-file", "filesystem-create",
                "filesystem-register", "filesystem-set-2304", "drop-file",
                "filesystem-set-1911", "drop-finish", "console"},
               "drop callback switch and restoration");
    f.runtime.calls.clear();
    f.runtime.files = {"C:\\again.txt"};
    f.runtime.created_callback = 0u;
    (void)f.Send(0x233u, 0x444u);
    CheckCalls(f.runtime,
               {"filesystem-get", "drop-count", "drop-file",
                "filesystem-create", "filesystem-find",
                "filesystem-set-2305", "filesystem-set-1911",
                "drop-finish", "console"},
               "drop falls back to existing UNCD callback");
}

#if defined(_WIN32) && !defined(_WIN64)
struct WindowServices final : pal4::IWin32GameWindowServices {
    std::uint32_t InitializeGameSystems() override { return 1u; }
    std::uint32_t InitializeDebugWindow() override { return 0u; }
    void DestroyDebugWindow(std::uint32_t) override {}
    void ShutdownGameSystems() override {}
    std::uint32_t Log(std::string_view, std::string_view,
                      std::uint32_t) override { return 1u; }
};

void TestNativeWindowMessageBinding() {
    Fixture f;
    f.runtime.native = true;
    pal4::Win32WindowMessageBinding binding(f.processor, f.runtime);
    const auto instance = GetModuleHandleA(nullptr);
    WNDCLASSEXA window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.hInstance = instance;
    window_class.lpszClassName = "PAL4-WndProc-Binding-Test";
    window_class.lpfnWndProc = reinterpret_cast<WNDPROC>(
        static_cast<std::uintptr_t>(binding.ProcedureAddress()));
    Check(RegisterClassExA(&window_class) != 0u,
          "native message class registration");
    const auto window = CreateWindowExA(
        0u, window_class.lpszClassName, "PAL4 message test",
        WS_OVERLAPPED, 0, 0, 320, 240, nullptr, nullptr, instance, nullptr);
    Check(window != nullptr && f.runtime.observed_window ==
              static_cast<std::uint32_t>(
                  reinterpret_cast<std::uintptr_t>(window)) &&
              f.mode.ReadDword(108u) == 1024u,
          "real WM_CREATE reaches reviewed message processor");
    (void)SendMessageA(window, WM_CHAR, 'A', 0u);
    Check(!f.runtime.keyboard_events.empty() &&
              f.runtime.keyboard_events.back() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x18u, 'A'},
          "real keyboard message reaches reviewed input path");
    Check(DestroyWindow(window) != FALSE && (f.config[3] & 0xffu) == 1u,
          "real WM_DESTROY reaches reviewed shutdown branch");
    Check(UnregisterClassA(window_class.lpszClassName, instance) != FALSE,
          "native message class unregistered");
}

void TestNativeWindowMessageRuntime() {
    Fixture f;
    pal4::WindowObjectState window_state;
    pal4::GameConfigWords window_config{};
    pal4::GameWindowLifecycle window_lifecycle{
        window_state, f.game, window_config};
    pal4::RandomNumberGenerator random;
    WindowServices services;
    pal4::Win32GameWindowRuntime window_runtime{f.game, random, services};
    pal4::Win32WindowMessageRuntime message_runtime{
        window_lifecycle, window_runtime, f.runtime};
    pal4::Win32WindowMessageBinding binding(f.processor, message_runtime);
    const auto instance = GetModuleHandleA(nullptr);
    WNDCLASSEXA window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.hInstance = instance;
    window_class.lpszClassName = "PAL4-Native-Message-Runtime-Test";
    window_class.lpfnWndProc = reinterpret_cast<WNDPROC>(
        static_cast<std::uintptr_t>(binding.ProcedureAddress()));
    Check(RegisterClassExA(&window_class) != 0u,
          "native runtime class registration");
    const auto window = CreateWindowExA(
        0u, window_class.lpszClassName, "PAL4 native runtime",
        WS_OVERLAPPED, 0, 0, 320, 240, nullptr, nullptr, instance, nullptr);
    Check(window != nullptr &&
              (GetWindowLongA(window, GWL_EXSTYLE) & WS_EX_ACCEPTFILES) != 0 &&
              f.mode.ReadDword(108u) != 0u,
          "real WM_CREATE enables drop and snapshots display mode");
    (void)SendMessageA(window, WM_CHAR, 'B', 0u);
    Check(!f.runtime.keyboard_events.empty() &&
              f.runtime.keyboard_events.back() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x18u, 'B'},
          "native platform runtime delegates keyboard to game service");
    f.game.Write(0x20u, 1u);
    (void)SendMessageA(window, WM_LBUTTONDOWN, MK_LBUTTON, 0u);
    Check(GetCapture() == window &&
              f.runtime.mouse_events.back() == 5u,
          "mouse press captures real HWND and delegates input");
    (void)SendMessageA(window, WM_LBUTTONUP, 0u, 0u);
    Check(GetCapture() == nullptr &&
              f.runtime.mouse_events.back() == 6u,
          "mouse release clears real capture");

    constexpr char kDropPath[] = "C:\\PAL4\\native.txt";
    const auto drop = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                  sizeof(DROPFILES) + sizeof(kDropPath));
    Check(drop != nullptr, "drop allocation");
    auto* payload = static_cast<std::uint8_t*>(GlobalLock(drop));
    Check(payload != nullptr, "drop lock");
    auto* header = reinterpret_cast<DROPFILES*>(payload);
    header->pFiles = sizeof(DROPFILES);
    std::memcpy(payload + sizeof(DROPFILES), kDropPath, sizeof(kDropPath));
    (void)GlobalUnlock(drop);
    const auto drop_handle = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(drop));
    Check(message_runtime.DroppedFileCount(drop_handle) == 1u &&
              message_runtime.DroppedFile(drop_handle, 0u) == kDropPath,
          "Shell32 reads the same ANSI drop payload as the game WndProc");
    message_runtime.FinishDrop(drop_handle);

    Check(DestroyWindow(window) != FALSE && (f.config[3] & 0xffu) == 1u,
          "native runtime handles real window destruction");
    Check(UnregisterClassA(window_class.lpszClassName, instance) != FALSE,
          "native runtime class unregistered");
}
#endif

}  // namespace

int main() {
    {
        pal4::WindowMessageConfig config{};
        config[0] = 800u;
        std::array<std::byte, pal4::WindowMessageConfig::kBytes + 1u> bytes{};
        auto unaligned = std::span(bytes).subspan(1u);
        Check(!config.BindExternal(0x1234u, unaligned.first(
                  pal4::WindowMessageConfig::kBytes - 1u)),
              "config backing rejects undersized storage");
        Check(config.BindExternal(0x1234u, unaligned) &&
                  config.Address() == 0x1234u,
              "config accepts unaligned native backing");
        std::uint32_t raw = 0u;
        std::memcpy(&raw, unaligned.data(), sizeof(raw));
        Check(raw == 800u, "config binding copies existing values");
        constexpr std::uint32_t external_value = 0x12345678u;
        std::memcpy(unaligned.data() + 31u * 4u, &external_value,
                    sizeof(external_value));
        Check(config[31] == external_value &&
                  config.data()[31] == external_value,
              "config reads direct native writes through words and data");
        config[3] = config[31];
        std::memcpy(&raw, unaligned.data() + 3u * 4u, sizeof(raw));
        Check(raw == external_value,
              "config word assignment mirrors native storage");
        config.UnbindExternal();
        Check(config.Address() == 0u && config[31] == external_value &&
                  config[3] == external_value,
              "config unbind retains final values");
    }
    TestScanAndFallback();
    TestWindowAndActivation();
    TestKeyboard();
    TestMouse();
    TestSizingAndDrop();
#if defined(_WIN32) && !defined(_WIN64)
    TestNativeWindowMessageBinding();
    TestNativeWindowMessageRuntime();
#endif
    return EXIT_SUCCESS;
}
