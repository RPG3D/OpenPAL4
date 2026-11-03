#include "pal4/native_renderware_game_systems_runtime.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_game_systems_shutdown_runtime.h"
#include "pal4/native_video_device_runtime.h"
#include "pal4/native_virtual_file_system_runtime.h"
#include "pal4/renderware_file_position_api_native.h"
#include "pal4/win32_game_window_runtime.h"

#include <windows.h>
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <rtfsyst.h>

#include <cstdint>
#include <array>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#undef GetCurrentDirectory
#undef EnumDisplaySettings
#undef ChangeDisplaySettings

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

RECT SecondaryDisplayBounds() {
    RECT bounds{};
    const auto find = [](HMONITOR monitor, HDC, LPRECT, LPARAM context) -> BOOL {
        MONITORINFO info{};
        info.cbSize = sizeof(info);
        if (!GetMonitorInfoA(monitor, &info) ||
            (info.dwFlags & MONITORINFOF_PRIMARY) != 0u ||
            info.rcMonitor.right - info.rcMonitor.left != 1600 ||
            info.rcMonitor.bottom - info.rcMonitor.top != 900)
            return TRUE;
        *reinterpret_cast<RECT*>(context) = info.rcMonitor;
        return FALSE;
    };
    (void)EnumDisplayMonitors(nullptr, nullptr, find,
                              reinterpret_cast<LPARAM>(&bounds));
    Check(bounds.right - bounds.left == 1600 &&
              bounds.bottom - bounds.top == 900,
          "secondary 1600x900 display is available");
    return bounds;
}

void ActivateTestWindow(HWND window) {
    const auto foreground = GetForegroundWindow();
    const auto foreground_thread = foreground != nullptr
                                       ? GetWindowThreadProcessId(foreground, nullptr)
                                       : 0u;
    const auto current_thread = GetCurrentThreadId();
    const bool attached = foreground_thread != 0u &&
                          foreground_thread != current_thread &&
                          AttachThreadInput(current_thread, foreground_thread,
                                            TRUE) != FALSE;
    (void)BringWindowToTop(window);
    (void)SetForegroundWindow(window);
    if (attached)
        (void)AttachThreadInput(current_thread, foreground_thread, FALSE);
    Check(GetForegroundWindow() == window,
          "native game window receives input focus");
}

std::uint32_t tga_read_count{};
RwImage* ReadTestTga(const char*) {
    ++tga_read_count;
    auto* image = RwImageCreate(1, 1, 32);
    if (image && !RwImageAllocatePixels(image)) {
        (void)RwImageDestroy(image);
        return nullptr;
    }
    return image;
}

class RemainingServices final : public pal4::IGameSystemsRuntime {
public:
    void InitializeAllocator(std::uint32_t table) override {
        Check(table == pal4::GameSystemsInitializer::kAllocatorTable,
              "allocator callback table address");
    }
    std::uint32_t EngineInit(std::uint32_t, std::uint32_t,
                             std::uint32_t) override { std::abort(); }
    std::uint32_t AttachPlugins() override { std::abort(); }
    std::uint32_t BindInputCallback(std::uint32_t selector,
                                    std::uint32_t callback) override {
        Check((selector == 0u && callback == 0x409630u) ||
              (selector == 1u && callback == 0x409670u),
              "input callback selection");
        return 1u;
    }
    std::uint32_t OpenEngine(std::uint32_t) override { std::abort(); }
    void ShutdownEngine() override { std::abort(); }
    std::uint32_t SelectDevice() override { std::abort(); }
    void SetRefreshRate(std::uint32_t) override { std::abort(); }
    void EnableDialogBoxMode(std::uint32_t) override { std::abort(); }
    std::uint32_t StartEngine() override { std::abort(); }
    void CloseEngine() override { std::abort(); }
    void StopEngine() override { std::abort(); }
    std::uint32_t GetD3D9Caps() override { std::abort(); }
    std::uint32_t GetCurrentD3DDevice() override { std::abort(); }
    std::uint32_t InitializeImageHandlers() override { std::abort(); }
    std::uint32_t OpenCharset() override { std::abort(); }
    std::uint32_t OpenFileSystemManager(std::uint32_t) override { std::abort(); }
    std::uint32_t InitializeVirtualFileSystem() override { std::abort(); }
    std::uint32_t InitializeDirectInput(std::uint32_t,
                                        std::uint32_t, std::uint32_t) override {
        std::abort();
    }
    void SetInputParameters(std::uint32_t, std::uint32_t,
                            std::uint32_t, std::uint32_t) override {
        std::abort();
    }
    void SetInputScalar(std::uint32_t) override { std::abort(); }
    std::uint32_t InitializeLogger() override { return 1u; }
    void AppendLog(std::uint32_t logger, std::string_view,
                   std::string_view, std::uint32_t) override {
        Check(logger == 1u, "startup logger identity");
    }

};

class NoSubsystemVideoRuntime final : public pal4::NativeVideoDeviceRuntime {
public:
    using NativeVideoDeviceRuntime::NativeVideoDeviceRuntime;
    std::uint32_t GetNumSubSystems() override { return 0u; }
};

class NoDirectoryFileSystemRuntime final
    : public pal4::NativeVirtualFileSystemRuntime {
public:
    using NativeVirtualFileSystemRuntime::NativeVirtualFileSystemRuntime;
    std::uint32_t GetCurrentDirectory(
        std::uint32_t, std::string&) override { return 0u; }
};

class PackageServices final : public pal4::IPackageFileMountRuntime {
public:
    std::uint32_t InitializePackFileManager() override {
        ++init_count;
        return 0x1234u;
    }
    void ProcessPackFiles(const std::uint32_t manager,
                          const std::uint32_t file_system) override {
        Check(manager == 0x1234u && file_system != 0u &&
              file_system == static_cast<std::uint32_t>(
                  reinterpret_cast<std::uintptr_t>(
                      RtFSManagerGetDefaultFileSystem())),
              "PK manager receives the selected SDK filesystem");
        ++process_count;
    }
    std::uint32_t init_count = 0u;
    std::uint32_t process_count = 0u;
};

class ShutdownLogger final : public pal4::IGameShutdownLogger {
public:
    std::uint32_t InitializeLogger() override { return 0x1234u; }
    std::int32_t AppendLog(
        const std::uint32_t logger, const std::string_view format,
        const std::string_view source_file,
        const std::uint32_t source_line) override {
        Check(logger == 0x1234u &&
              format.find("Terminate3D success") != std::string_view::npos &&
              source_file.find("rwc_Initialize.cpp") != std::string_view::npos &&
              source_line == 0x493u,
              "reviewed game shutdown log");
        ++calls;
        return 73;
    }
    std::uint32_t calls = 0u;
};

class WindowSystemsServices final : public pal4::IWin32GameWindowServices,
                                    public pal4::IGameShutdownLogger {
public:
    WindowSystemsServices(pal4::GameConfigWords& config,
                          pal4::GameRenderState& state,
                          pal4::NativeRenderWareGameSystemsRuntime& renderware)
        : config_(config), state_(state), renderware_(renderware) {}

    std::uint32_t InitializeGameSystems() override {
        ++initializations;
        (void)SetForegroundWindow(reinterpret_cast<HWND>(
            static_cast<std::uintptr_t>(state_.Read(0u))));
        return pal4::GameSystemsInitializer(config_, state_)
            .Initialize(renderware_);
    }
    std::uint32_t InitializeDebugWindow() override { return 0u; }
    void DestroyDebugWindow(std::uint32_t) override {}
    void ShutdownGameSystems() override {
        pal4::NativeGameSystemsShutdownRuntime shutdown(
            state_, renderware_, *this);
        Check(pal4::GameSystemsShutdown{}.Shutdown(shutdown) == 73 &&
              shutdown.DestroyedDictionaryCount() != 0u,
              "game window closes native RenderWare through reviewed shutdown");
        ++shutdowns;
    }
    std::uint32_t Log(std::string_view, std::string_view,
                      std::uint32_t) override { return 1u; }
    std::uint32_t InitializeLogger() override { return 0x1234u; }
    std::int32_t AppendLog(std::uint32_t logger, std::string_view format,
                           std::string_view, std::uint32_t line) override {
        Check(logger == 0x1234u &&
              format.find("Terminate3D success") != std::string_view::npos &&
              line == 0x493u, "game window shutdown log");
        return 73;
    }

    std::uint32_t initializations = 0u;
    std::uint32_t shutdowns = 0u;

private:
    pal4::GameConfigWords& config_;
    pal4::GameRenderState& state_;
    pal4::NativeRenderWareGameSystemsRuntime& renderware_;
};

void TestGameWindowLifecycle(
    const std::optional<std::filesystem::path>& original_root) {
    pal4::GameConfigWords config{};
    config[0] = 640u;
    config[1] = 480u;
    DEVMODEA display{};
    display.dmSize = sizeof(display);
    Check(EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &display) != 0,
          "game window display settings");
    config[2] = display.dmBitsPerPel;
    pal4::GameRenderState state{0x9000u};
    state.Write(4u, static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr))));
    state.Write(0x58cu, 1u);
    RemainingServices remaining;
    pal4::NativeVideoDeviceRuntime video(remaining);
    pal4::NativePkFileManagerRuntime packages(
        pal4::RenderWareFilePositionApi());
    pal4::NativeVirtualFileSystemRuntime file_system(remaining, packages);
    pal4::NativeRenderWareGameSystemsRuntime renderware(
        remaining, config, state, video, file_system, &ReadTestTga,
        nullptr, [&] { packages.Shutdown(); });
    WindowSystemsServices services(config, state, renderware);
    pal4::RandomNumberGenerator random;
    pal4::WindowObjectState window_state;
    pal4::GameWindowLifecycle lifecycle(window_state, state, config);
    pal4::Win32GameWindowRuntime window_runtime(state, random, services);
    const auto procedure = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&DefWindowProcA));
    constexpr std::string_view class_name = "PAL4_Rewrite_GameSystems";
    Check(lifecycle.Create(class_name, "PAL4 native game systems",
                           procedure, true, nullptr, 0u, 0u,
                           window_runtime) == 1u &&
          services.initializations == 1u &&
          IsWindow(reinterpret_cast<HWND>(
              static_cast<std::uintptr_t>(state.Read(0u)))) != FALSE &&
          state.Read(0x55cu) != 0u &&
          packages.ManagerAddress() != 0u,
          "reviewed game window creation starts SDK and PK manager");
    const auto game_window = reinterpret_cast<HWND>(
        static_cast<std::uintptr_t>(state.Read(0u)));
    MONITORINFOEXA window_display{};
    window_display.cbSize = sizeof(window_display);
    Check(GetMonitorInfoA(MonitorFromWindow(game_window, MONITOR_DEFAULTTONULL),
                          &window_display) != FALSE,
          "game window has a display device");
    pal4::DisplayModeRecord selected_mode;
    video.EnumDisplaySettings(ENUM_CURRENT_SETTINGS, selected_mode);
    DEVMODEA native_mode{};
    std::memcpy(&native_mode, selected_mode.Bytes().data(), sizeof(native_mode));
    if (native_mode.dmPosition.x != window_display.rcMonitor.left ||
        native_mode.dmPosition.y != window_display.rcMonitor.top ||
        native_mode.dmPelsWidth != 1600u ||
        native_mode.dmPelsHeight != 900u)
        std::fprintf(stderr,
                     "window display=%s origin=%ld,%ld; mode origin=%ld,%ld size=%lu,%lu\n",
                     window_display.szDevice, window_display.rcMonitor.left,
                     window_display.rcMonitor.top, native_mode.dmPosition.x,
                     native_mode.dmPosition.y, native_mode.dmPelsWidth,
                     native_mode.dmPelsHeight);
    Check(native_mode.dmPosition.x == window_display.rcMonitor.left &&
              native_mode.dmPosition.y == window_display.rcMonitor.top &&
              native_mode.dmPelsWidth == 1600u &&
              native_mode.dmPelsHeight == 900u,
          "video mode queries use the game window's secondary display");
    Check(video.ChangeDisplaySettings(selected_mode, CDS_TEST) ==
              DISP_CHANGE_SUCCESSFUL,
          "target display accepts a mode validation without applying it");
    if (original_root) {
        const auto prior_directory = std::filesystem::current_path();
        std::filesystem::current_path(*original_root);
        const auto loose_path = (*original_root / "config.cfg").string();
        auto* loose_file = RwFopen(loose_path.c_str(), "rb");
        Check(loose_file != nullptr, "RenderWare opens original loose file");
        Check(RwFseek(loose_file, 7, 0) == 0,
              "RenderWare positions original loose file");
        const auto loose_handle = static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(loose_file));
        Check(packages.GetScriptSize(loose_handle) ==
                  std::filesystem::file_size("config.cfg") &&
              RwFtell(loose_file) == 7,
              "PK script size uses SDK seek/tell and restores file cursor");
        (void)RwFclose(loose_file);
        Check(packages.LoadAllScripts() == 1u &&
              packages.LoadedPackageCount() == 9u,
              "game window startup loads original CPK packages");
        const auto handle = packages.OpenScriptMember(
            5u, "fonts\\fontPuzzle_0.imageset");
        const auto ui_script = packages.ManagerAddress() +
            pal4::PkFileManager::kFirstScriptOffset +
            5u * pal4::PkFileManager::kScriptStride;
        auto& scripts = packages.Scripts();
        constexpr std::uint32_t key = 0x80000001u;
        Check(handle != 0u &&
              pal4::ScriptManagerCore::InsertScript(
                  ui_script, key, handle, scripts) != 0u,
              "game window registers original UI member");
        char prefix[9]{};
        Check(pal4::ScriptManagerCore::ReadScript(
                  ui_script, key, prefix, 8u, scripts) == 8u &&
              std::string_view(prefix).find("<?xml") == 0u,
              "game window reads original UI XML through script tree");
        std::filesystem::current_path(prior_directory);
    }
    Check(lifecycle.Destroy(window_runtime) != 0u &&
          services.shutdowns == 1u &&
          packages.ManagerAddress() == 0u &&
          packages.ArchiveCount() == 0u &&
          packages.AllocationCount() == 0u &&
          renderware.Input().ReadDword(0u) == 0u,
          "reviewed game window destruction releases SDK and PK manager");
    WNDCLASSEXA registered{};
    registered.cbSize = sizeof(registered);
    Check(GetClassInfoExA(GetModuleHandleA(nullptr),
                          class_name.data(), &registered) == FALSE,
          "game window class unregistered after native shutdown");
}

}  // namespace

int main(const int argc, char** argv) {
    Check(argc == 2 || argc == 3, "select a test case");
    const std::string_view test_case(argv[1]);
    if (test_case == "window_lifecycle" ||
        test_case == "window_lifecycle_original") {
        Check((test_case == "window_lifecycle_original") == (argc == 3),
              "window lifecycle asset path matches test case");
        TestGameWindowLifecycle(argc == 3 ?
            std::optional<std::filesystem::path>{argv[2]} : std::nullopt);
        return EXIT_SUCCESS;
    }
    Check(test_case == "success" || test_case == "select_failure" ||
          test_case == "vfs_failure" || test_case == "package_enabled" ||
          test_case == "native_tga" || test_case == "package_original",
          "known test case");
    Check((test_case == "package_original") == (argc == 3),
          "original package case receives installation path");
    const auto display_bounds = SecondaryDisplayBounds();
    const auto window = CreateWindowA(
        "STATIC", "PAL4 game systems native", WS_OVERLAPPEDWINDOW,
        display_bounds.left + (1600 - 640) / 2,
        display_bounds.top + (900 - 480) / 2,
        640, 480, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    Check(window != nullptr, "native game window");
    ShowWindow(window, SW_SHOWNORMAL);
    RECT window_bounds{};
    Check(GetWindowRect(window, &window_bounds) != FALSE &&
              window_bounds.left >= display_bounds.left &&
              window_bounds.top >= display_bounds.top &&
              window_bounds.right <= display_bounds.right &&
              window_bounds.bottom <= display_bounds.bottom,
          "native game systems window stays on secondary display");
    ActivateTestWindow(window);
    pal4::GameConfigWords config{};
    config[0] = 640u;
    config[1] = 480u;
    DEVMODEA display{};
    display.dmSize = sizeof(display);
    Check(EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &display) != 0,
          "current display settings");
    config[2] = display.dmBitsPerPel;
    pal4::GameRenderState state{0x9000u};
    state.Write(0u, static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(window)));
    state.Write(4u, static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr))));
    if (test_case == "package_enabled" || test_case == "package_original")
        state.Write(0x58cu, 1u);
    RemainingServices services;
    pal4::NativeVideoDeviceRuntime video(services);
    NoSubsystemVideoRuntime failing_video(services);
    pal4::IVideoDeviceRuntime& selected_video =
        test_case == "select_failure"
            ? static_cast<pal4::IVideoDeviceRuntime&>(failing_video)
            : static_cast<pal4::IVideoDeviceRuntime&>(video);
    PackageServices packages;
    pal4::NativePkFileManagerRuntime native_packages(
        pal4::RenderWareFilePositionApi());
    pal4::IPackageFileMountRuntime& selected_packages =
        test_case == "package_enabled" || test_case == "package_original"
            ? static_cast<pal4::IPackageFileMountRuntime&>(native_packages)
            : static_cast<pal4::IPackageFileMountRuntime&>(packages);
    pal4::NativeVirtualFileSystemRuntime file_system(services, selected_packages);
    NoDirectoryFileSystemRuntime failing_file_system(services, selected_packages);
    pal4::IVirtualFileSystemRuntime& selected_file_system =
        test_case == "vfs_failure"
            ? static_cast<pal4::IVirtualFileSystemRuntime&>(failing_file_system)
            : static_cast<pal4::IVirtualFileSystemRuntime&>(file_system);
    std::uint32_t package_shutdowns = 0u;
    pal4::NativeRenderWareGameSystemsRuntime runtime(
        services, config, state, selected_video, selected_file_system,
        test_case == "native_tga" ? pal4::ReadNativeGameTga : &ReadTestTga,
        nullptr, [&] {
            if (native_packages.ManagerAddress() == 0u) return;
            Check(RtFSManagerGetDefaultFileSystem() != nullptr,
                  "PK closes while SDK filesystem is still registered");
            native_packages.Shutdown();
            ++package_shutdowns;
        });
    const auto result = pal4::GameSystemsInitializer(config, state)
                            .Initialize(runtime);
    if (test_case == "success" || test_case == "package_enabled" ||
        test_case == "package_original" ||
        test_case == "native_tga") {
        Check(result == 1u, "game systems initializer success");
        Check(RwEngineGetPluginOffset(8449u) >= 0 &&
              RpAtomicGetPluginOffset(8452u) > 0,
              "all game plugins attached");
        Check(state.Read(0x55cu) != 0u && state.Read(0x560u) != 0u,
              "D3D capability and device pointers stored");
        Check(state.Read(0x530u) > 0u && state.Read(0x534u) < state.Read(0x530u) &&
              config[0] > 0u && config[1] > 0u,
              "reviewed video selector used native device and client size");
        const auto& input = runtime.Input();
        Check(input.ReadDword(0u) != 0u && input.ReadDword(4u) != 0u &&
              input.ReadDword(8u) != 0u,
              "game startup created native DirectInput devices");
        Check(input.ReadDword(0x186cu) == 1u &&
              input.ReadDword(0x1870u) == 1u,
              "game startup acquired mouse and keyboard devices");
        Check(input.ReadDword(0x10u) == 0u && input.ReadDword(0x14u) == 0u &&
              input.ReadDword(0x18u) == config[0] &&
              input.ReadDword(0x1cu) == config[1] &&
              input.ReadDword(0x0cu) == 0x3f800000u,
              "game startup stored input dimensions and scalar");
        char name[] = "rwc";
        Check(RtFSManagerGetFileSystemFromName(name) != nullptr &&
              RtFSManagerGetDefaultFileSystem() != nullptr,
              "game VFS registered SDK file systems and selected a default");
        const auto tga_name = std::string("pal4_startup_callback_") +
                              std::to_string(GetCurrentProcessId()) + ".tga";
        {
            std::ofstream fixture(tga_name, std::ios::binary);
            if (test_case == "native_tga") {
                const std::array<std::uint8_t, 34> bytes{
                    0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    2, 0, 2, 0, 32, 0,
                    0, 0, 255, 255, 0, 255, 0, 255,
                    255, 0, 0, 255, 255, 255, 255, 255};
                fixture.write(reinterpret_cast<const char*>(bytes.data()),
                              bytes.size());
            } else {
                fixture.put('\0');
            }
            Check(static_cast<bool>(fixture), "TGA callback fixture file");
        }
        auto* tga = RwImageRead(tga_name.c_str());
        std::filesystem::remove(tga_name);
        Check(tga != nullptr, "game image registration reached the TGA callback");
        if (test_case == "native_tga") {
            Check(tga_read_count == 0u && RwImageGetWidth(tga) == 2 &&
                  RwImageGetHeight(tga) == 2 && RwImageGetDepth(tga) == 32,
                  "native game TGA image dimensions");
            const auto* pixels = RwImageGetPixels(tga);
            const auto stride = RwImageGetStride(tga);
            const std::array<std::uint8_t, 8> top{
                0, 0, 255, 255, 255, 255, 255, 255};
            const std::array<std::uint8_t, 8> bottom{
                255, 0, 0, 255, 0, 255, 0, 255};
            Check(std::memcmp(pixels, top.data(), top.size()) == 0 &&
                  std::memcmp(pixels + stride, bottom.data(), bottom.size()) == 0,
                  "native game TGA channels and bottom-up rows");
        } else {
            Check(tga_read_count == 1u, "injected TGA callback count");
        }
        Check(RwImageDestroy(tga) != 0, "test TGA image cleanup");
        if (test_case == "native_tga") {
            std::array<std::uint8_t, 34> top_down{
                0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                2, 0, 2, 0, 32, 0x20,
                255, 0, 0, 255, 255, 255, 255, 255,
                0, 0, 255, 255, 0, 255, 0, 255};
            {
                std::ofstream fixture(tga_name, std::ios::binary);
                fixture.write(reinterpret_cast<const char*>(top_down.data()),
                              top_down.size());
                Check(static_cast<bool>(fixture), "top-down TGA fixture");
            }
            auto* top_image = RwImageRead(tga_name.c_str());
            Check(top_image != nullptr, "top-down native TGA read");
            const auto* pixels = RwImageGetPixels(top_image);
            const auto stride = RwImageGetStride(top_image);
            const std::array<std::uint8_t, 8> top{
                0, 0, 255, 255, 255, 255, 255, 255};
            const std::array<std::uint8_t, 8> bottom{
                255, 0, 0, 255, 0, 255, 0, 255};
            Check(std::memcmp(pixels, top.data(), top.size()) == 0 &&
                  std::memcmp(pixels + stride, bottom.data(), bottom.size()) == 0,
                  "top-down native TGA rows");
            Check(RwImageDestroy(top_image) != 0, "top-down TGA cleanup");
            {
                std::ofstream fixture(tga_name, std::ios::binary);
                fixture.write(reinterpret_cast<const char*>(top_down.data()), 18);
                Check(static_cast<bool>(fixture), "truncated TGA fixture");
            }
            Check(RwImageRead(tga_name.c_str()) == nullptr,
                  "truncated native TGA rejected");
            std::filesystem::remove(tga_name);
        }
        char directory[MAX_PATH]{};
        Check(GetCurrentDirectoryA(MAX_PATH, directory) != 0u &&
              directory[1] == ':', "current drive directory");
        char drive_name[]{directory[0], '\0'};
        Check(RtFSManagerGetDefaultFileSystem() ==
              RtFSManagerGetFileSystemFromName(drive_name),
              "game VFS selected the current drive alias");
        if (test_case == "package_enabled" || test_case == "package_original") {
            Check(native_packages.ManagerAddress() != 0u &&
                  native_packages.BoundFileSystem() == static_cast<std::uint32_t>(
                      reinterpret_cast<std::uintptr_t>(
                          RtFSManagerGetDefaultFileSystem())) &&
                  native_packages.ScriptInstanceCount() ==
                      pal4::PkFileManager::kScriptCount &&
                  native_packages.ArchiveCount() ==
                      pal4::PkFileManager::kScriptCount,
                  "PK branch constructs and binds native script manager");
            if (test_case == "package_original") {
                const auto prior_directory = std::filesystem::current_path();
                std::filesystem::current_path(argv[2]);
                Check(native_packages.LoadAllScripts() == 1u &&
                      native_packages.LoadedPackageCount() == 9u,
                      "game startup loads nine original CPK packages");
                const auto handle = native_packages.OpenScriptMember(
                    5u, "fonts\\fontPuzzle_0.imageset");
                auto& scripts = native_packages.Scripts();
                const auto ui_script = native_packages.ManagerAddress() +
                    pal4::PkFileManager::kFirstScriptOffset +
                    5u * pal4::PkFileManager::kScriptStride;
                constexpr std::uint32_t key = 0x80000001u;
                Check(handle != 0u &&
                      pal4::ScriptManagerCore::InsertScript(
                          ui_script, key, handle, scripts) != 0u,
                      "SDK-bound PK manager registers original UI member");
                char xml_prefix[9]{};
                Check(pal4::ScriptManagerCore::ReadScript(
                          ui_script, key, xml_prefix, 8u, scripts) == 8u &&
                      std::string_view(xml_prefix).find("<?xml") == 0u,
                      "SDK-bound script manager reads original UI XML");
                Check(pal4::ScriptManagerCore::CloseScript(
                          ui_script, key, scripts) == 1u &&
                      pal4::ScriptManagerCore::RemoveScript(
                          ui_script, key, scripts) != 0u,
                      "SDK-bound script member and tree node clean up");
                std::filesystem::current_path(prior_directory);
            }
        } else {
            Check(packages.init_count == 0u && packages.process_count == 0u,
                  "PK manager gate follows game render state");
        }
        const auto& dialog = runtime.DialogModeResult();
        Check(dialog.requested_sdk_version == 31u &&
              dialog.create_device_calls == 1u &&
              (dialog.present_flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER) &&
              dialog.swap_effect == D3DSWAPEFFECT_DISCARD,
              "original dialog-mode device configuration");
        const auto installed_callback = RwTextureGetReadCallBack();
        Check(RwTexDictionaryCreate() != nullptr,
              "native shutdown has a live texture dictionary");
        ShutdownLogger shutdown_logger;
        pal4::NativeGameSystemsShutdownRuntime shutdown_runtime(
            state, runtime, shutdown_logger);
        Check(pal4::GameSystemsShutdown{}.Shutdown(shutdown_runtime) == 73 &&
              shutdown_logger.calls == 1u &&
              shutdown_runtime.DestroyedDictionaryCount() >= 1u,
              "reviewed shutdown destroys SDK dictionaries and logs result");
        Check(RwTextureGetReadCallBack() != installed_callback,
              "texture hook removed before engine stop");
        Check(native_packages.ScriptInstanceCount() == 0u &&
              native_packages.ArchiveCount() == 0u &&
              package_shutdowns ==
                  (test_case == "package_enabled" ||
                   test_case == "package_original" ? 1u : 0u),
              "PK scripts released before SDK filesystem closes");
        Check(runtime.Input().ReadDword(0u) == 0u &&
              runtime.Input().ReadDword(4u) == 0u &&
              runtime.Input().ReadDword(8u) == 0u,
              "native DirectInput devices released on shutdown");
    } else {
        Check(result == 0u, "game-system failure propagated");
        runtime.ShutdownEngine();
        Check(runtime.EngineInit(0u, 0u, 0x400000u) == 1u,
              "failed startup released the engine");
        runtime.ShutdownEngine();
    }
    Check(DestroyWindow(window) != 0, "native game window cleanup");
    return EXIT_SUCCESS;
}
