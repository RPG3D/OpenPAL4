#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>
#include "renderware_camera_cases.h"
#include "renderware_texture_cases.h"
#include "renderware_texture_loader_cases.h"
#include "renderware_frame_cases.h"
#include "renderware_vector_cases.h"
#include "renderware_device_cases.h"
#include "pal4/native_renderware_dialog_mode.h"
#include "renderware_reset_cases.h"
#include "renderware_presentation_cases.h"
#include "renderware_state_cases.h"
#include "renderware_raster_resource_cases.h"
#include "renderware_stream_cases.h"
#include "renderware_matrix_cases.h"
#include "renderware_im3d_cases.h"
#include "renderware_pipeline_cases.h"
#include "renderware_pipeline_edit_cases.h"
#include "renderware_vertex_buffer_cases.h"
#include "renderware_render_cache_cases.h"
#include "renderware_texture_alpha_cases.h"
#include "renderware_raster_lock_cases.h"
#include "renderware_raster_render_cases.h"
#include "renderware_raster_lifecycle_cases.h"
#include "renderware_native_format_cases.h"
#include "renderware_bounds_resample_cases.h"
#include "renderware_slist_cases.h"
#include "renderware_texture_stream_cases.h"
#include "renderware_dds_cases.h"
#include "renderware_world_cases.h"

namespace {
static_assert(sizeof(void*) == 4 && sizeof(RwBool) == 4);
static_assert(std::is_same_v<decltype(&RwEngineInit), RwBool (__cdecl *)(const RwMemoryFunctions*, RwUInt32, RwUInt32)>);
static_assert(std::is_same_v<decltype(&RwEngineGetCurrentSubSystem), RwInt32 (__cdecl *)(void)>);
static_assert(std::is_same_v<decltype(&RwEngineGetCurrentVideoMode), RwInt32 (__cdecl *)(void)>);
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::unordered_set<void*> allocations;
bool fail_next{}, invalid_free{}, fail_constructor{};
std::vector<int> events;
void* Allocate(size_t n, RwUInt32) {
    if (fail_next) { fail_next = false; return nullptr; }
    void* p = std::malloc(n); if (p) allocations.insert(p); return p;
}
void Free(void* p) {
    if (p && allocations.erase(p) != 1) invalid_free = true;
    std::free(p);
}
void* Reallocate(void* p, size_t n, RwUInt32 hint) {
    if (!p) return Allocate(n, hint);
    if (fail_next) { fail_next = false; return nullptr; }
    if (!n) { Free(p); return nullptr; }
    if (allocations.erase(p) != 1) invalid_free = true;
    void* result = std::realloc(p, n);
    allocations.insert(result ? result : p); return result;
}
void* Callocate(size_t count, size_t size, RwUInt32 hint) {
    if (size && count > static_cast<size_t>(-1) / size) return nullptr;
    void* p = Allocate(count * size, hint); if (p) std::memset(p, 0, count * size); return p;
}
void* FirstConstruct(void* object, RwInt32 offset, RwInt32 size) {
    Require(size == 4 && object == RwEngineInstance, "plugin object/size mismatch");
    *reinterpret_cast<int*>(static_cast<char*>(object) + offset) = 0x1357;
    events.push_back(1); return object;
}
void* FirstDestruct(void* object, RwInt32 offset, RwInt32) {
    Require(*reinterpret_cast<int*>(static_cast<char*>(object) + offset) == 0x1357, "plugin storage lifetime mismatch");
    events.push_back(-1); return object;
}
void* SecondConstruct(void* object, RwInt32, RwInt32) {
    events.push_back(2); return fail_constructor ? nullptr : object;
}
void* SecondDestruct(void* object, RwInt32, RwInt32) { events.push_back(-2); return object; }

class Session {
public:
    bool initialized{}, opened{}, started{};
    HWND window{};
    ~Session() {
        if (started) RwEngineStop();
        if (opened) RwEngineClose();
        if (initialized) RwEngineTerm();
        if (window) DestroyWindow(window);
    }
    void Init(RwUInt32 flags) {
        const RwMemoryFunctions memory{Allocate, Free, Reallocate, Callocate};
        Require(RwEngineGetVersion() == 0x37002, "SDK version mismatch");
        Require(RwEngineInit(&memory, flags, 4 * 1024 * 1024), "engine init failed");
        initialized = true;
        const auto a = RwEngineRegisterPlugin(4, 0x123401, FirstConstruct, FirstDestruct);
        const auto b = RwEngineRegisterPlugin(4, 0x123402, SecondConstruct, SecondDestruct);
        Require(a >= 0 && b >= a + 4, "plugin offsets overlap");
        Require(RwEngineGetPluginOffset(0x123401) == a && RwEngineGetPluginOffset(0x123402) == b,
                "engine plugin registry lookup mismatch");
        Require(RwEngineGetPluginOffset(0x123499) == -1, "absent plugin offset mismatch");
        Require(events.empty(), "plugin constructor ran before Start");
        window = pal4::test::CreateTestWindow("STATIC", "PAL4 engine boundary", WS_OVERLAPPEDWINDOW, 64, 64, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
        Require(window != nullptr, "hidden window failed");
    }
    bool Open() { RwEngineOpenParams params{window}; opened = RwEngineOpen(&params) != 0; return opened; }
    void Close() {
        if (started) { Require(RwEngineStop(), "Stop failed"); started = false; }
        if (opened) { Require(RwEngineClose(), "Close failed"); opened = false; }
        Require(RwEngineTerm(), "Term failed"); initialized = false;
    }
};
void Run(std::string_view name) {
    bool handled = name=="lifecycle" || name=="no_freelists" || name=="constructor_failure" || name=="cold_constructor_failure" || name=="allocation_failure";
    Session session;
    pal4::NativeRenderWareDialogMode dialog_mode;
    if (name == "device_dialog_mode_hook_probe") {
        Require(dialog_mode.PrepareBeforeOpen(), "dialog mode hook prepare");
        Require(dialog_mode.Cancel(), "dialog mode hook cancel");
        Require(dialog_mode.PrepareBeforeOpen(), "dialog mode hook reinstall");
    }
    session.Init(name == "no_freelists" || name == "frame_failure" || name == "im3d_matrix_lifetime" || name == "pipeline_lifetime" || (name == "pipeline_edit_game" || name == "pipeline_edit_matfx_world") ? rwENGINEINITNOFREELISTS : rwENGINEINITFREELISTS);
    if ((name == "pipeline_edit_game" || name == "pipeline_edit_matfx_world") || name == "pipeline_edit_atomic_node")
        pal4::test::RegisterPipelineEditWorldPlugin();
    if (name.starts_with("bounds_") || name.starts_with("world_") || (name == "camera_shadow_lifecycle" || name == "camera_shadow_atomic_flags" || name == "presentation_shadow_collision")) pal4::test::RegisterBoundsPlugin();
    if (name == "world_clump_plugin") pal4::test::RegisterClumpPluginBoundary();
    if (name.starts_with("texstream_")) pal4::test::RegisterTextureStreamPlugin();
    if (name == "texstream_plugin_registry") pal4::test::RegisterPluginRegistryStreamFixture();
    if (name.starts_with("dds_")) pal4::test::RegisterDdsPlugin();
    if (name.starts_with("texture_alpha_")) pal4::test::RegisterTextureAlphaPlugins();
    if (name.starts_with("camera_")) pal4::test::RegisterCameraPlugin();
    if (name == "presentation_shadow_collision") pal4::test::RegisterShadowCollisionPlugin();
    if (name == "camera_shadow_atomic_flags") pal4::test::RegisterShadowAtomicPlugins();
    if (name.starts_with("raster_")) pal4::test::RegisterRasterPlugin();
    if (name.starts_with("texture_")) pal4::test::RegisterTexturePlugin();
    if (name.starts_with("frame_")) pal4::test::RegisterFramePlugin();
    if (name == "allocation_failure") {
        const auto before = allocations.size();
        fail_next = true;
        Require(!session.Open() && !fail_next, "Open allocation failure not propagated");
        Require(allocations.size() == before && events.empty(), "failed Open leaked or constructed plugins");
    }
    Require(session.Open(), "engine Open failed");
    Require(!RwEngineTerm(), "Term must refuse an open engine");
    const auto count = RwEngineGetNumSubSystems();
    const auto current = RwEngineGetCurrentSubSystem();
    Require(count > 0 && current >= 0 && current < count, "subsystem query mismatch");
    RwSubSystemInfo info{};
    Require(RwEngineGetSubSystemInfo(&info, current) == &info && info.name[0], "subsystem info failed");
    Require(RwEngineSetSubSystem(current), "select current subsystem failed");
    const auto mode = RwEngineGetCurrentVideoMode();
    RwVideoMode video{};
    Require(mode >= 0 && RwEngineGetVideoModeInfo(&video, mode) == &video, "video query failed");
    Require(RwEngineSetVideoMode(mode), "select current video mode failed");
    if (name.starts_with("device_")) pal4::test::PrepareDeviceCase(name);
    if (name == "device_dialog_mode_hook_probe")
        Require(dialog_mode.EnableBeforeStart(true), "dialog mode enable");
    if (name == "constructor_failure" || name == "cold_constructor_failure") {
        if (name == "constructor_failure") {
            // The SDK saves the x87 control word only in FINALIZESTART.
            // A cold constructor failure skips that save, then STOP loads zero.
            // Keep that driver-crash probe separate; test retry after a completed
            // lifecycle here without changing the library or masking its fault.
            Require(RwEngineStart(), "warm-up Start failed"); session.started = true;
            Require(RwEngineStop(), "warm-up Stop failed"); session.started = false;
            Require(events == std::vector<int>({1, 2, -2, -1}), "warm-up lifecycle mismatch");
            events.clear();
        }
        fail_constructor = true;
        Require(!RwEngineStart(), "failed constructor not propagated");
        Require(events == std::vector<int>({1, 2, -1}), "constructor rollback order mismatch");
        events.clear(); fail_constructor = false;
    }
    Require(RwEngineStart(), "engine Start/retry failed"); session.started = true;
    if (name == "device_dialog_mode_hook_probe")
        Require(dialog_mode.FinishAfterStart(), "dialog mode startup verification");
    Require(events == std::vector<int>({1, 2}), "constructor order mismatch");
    if (name.starts_with("camera_")) { handled=true; pal4::test::RunCameraCase(name); }
    if (name.starts_with("texture_alpha_")) { handled=true; pal4::test::RunTextureAlphaCase(name); }
    else if (name.starts_with("texture_")) { handled=true; pal4::test::RunTextureCase(name); }
    if (name.starts_with("loader_")) { handled=true; pal4::test::RunTextureLoaderCase(name); }
    if (name.starts_with("frame_")) { handled=true; pal4::test::RunFrameCase(name, fail_next); }
    if (name.starts_with("vector_")) { handled=true; pal4::test::RunVectorCase(name); }
    if (name.starts_with("vertex_buffer_")) { handled=true; pal4::test::RunVertexBufferCase(name, session.window); }
    if (name.starts_with("render_cache_")) { handled=true; pal4::test::RunRenderCacheCase(name, session.window); }
    if (name.starts_with("device_")) { handled=true; pal4::test::RunDeviceCase(name); }
    if (name == "device_dialog_mode_hook_probe") {
        pal4::test::RunPresentationCase("presentation_draw", session.window);
        pal4::test::RunResetCase("reset_resize", session.window);
        pal4::test::RunDeviceCase(name);
    }
    if (name.starts_with("reset_")) { handled=true; pal4::test::RunResetCase(name, session.window); }
    if (name.starts_with("presentation_")) { handled=true; pal4::test::RunPresentationCase(name, session.window); }
    if (name.starts_with("state_")) { handled=true; pal4::test::RunStateCase(name); }
    if (name.starts_with("raster_lifecycle_")) { handled=true; pal4::test::RunRasterLifecycleCase(name,session.window); }
    else if (name.starts_with("raster_render_")) { handled=true; pal4::test::RunRasterRenderCase(name); }
    else if (name.starts_with("raster_lock_")) { handled=true; pal4::test::RunRasterLockCase(name); }
    else if (name.starts_with("raster_") || name.starts_with("resources_")) { handled=true; pal4::test::RunRasterResourceCase(name); }
    if (name.starts_with("stream_")) { handled=true; pal4::test::RunStreamCase(name, fail_next); }
    if (name.starts_with("native_format_")) { handled=true; pal4::test::RunNativeFormatCase(name); }
    if (name.starts_with("bounds_")||name.starts_with("resample_")) { handled=true; pal4::test::RunBoundsResampleCase(name,fail_next); }
    if (name.starts_with("slist_")) { handled=true; pal4::test::RunSListCase(name,fail_next); }
    if (name.starts_with("texstream_")) { handled=true; pal4::test::RunTextureStreamCase(name); }
    if (name.starts_with("dds_")) { handled=true; pal4::test::RunDdsCase(name); }
    if (name.starts_with("world_")) { handled=true; pal4::test::RunWorldCase(name); }
    if (name.starts_with("matrix_")) { handled=true; pal4::test::RunMatrixCase(name); }
    if (name.starts_with("im3d_")) { handled=true; pal4::test::RunIm3DCase(name, session.window, fail_next); }
    if (name.starts_with("pipeline_edit_")) { handled=true; pal4::test::RunPipelineEditCase(name, fail_next); }
    else if (name.starts_with("pipeline_")) { handled=true; pal4::test::RunPipelineCase(name, fail_next); }
    Require(handled,"unknown engine boundary case");
    // Public image allocation proves that built-in plugins are usable after Start.
    auto* image = RwImageCreate(1, 1, 32);
    Require(image && RwImageAllocatePixels(image), "started image plugin unusable");
    Require(RwImageDestroy(image), "image cleanup failed");
    session.Close();
    Require(events == std::vector<int>({1, 2, -2, -1}), "destructor order mismatch");
    Require(allocations.empty() && !invalid_free, "engine lifetime allocation mismatch");
}
}
int main(int argc, char** argv) {
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* p) -> LONG {
        HMODULE module{}; char path[MAX_PATH]{};
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            static_cast<LPCSTR>(p->ExceptionRecord->ExceptionAddress), &module);
        GetModuleFileNameA(module, path, MAX_PATH);
        std::fprintf(stderr, "native exception %08lX at %p; events:", p->ExceptionRecord->ExceptionCode,
                     p->ExceptionRecord->ExceptionAddress);
        for (int event : events) std::fprintf(stderr, " %d", event);
        std::fprintf(stderr, "\nmodule %s + %zx\n", path,
            reinterpret_cast<size_t>(p->ExceptionRecord->ExceptionAddress) - reinterpret_cast<size_t>(module));
        return EXCEPTION_EXECUTE_HANDLER;
    });
    try { Require(argc == 2, "expected case"); Run(argv[1]); std::puts("engine boundary passed"); return 0; }
    catch (const std::exception& e) { std::fprintf(stderr, "%s\n", e.what()); return 1; }
}
