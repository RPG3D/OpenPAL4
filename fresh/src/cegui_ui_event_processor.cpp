#include "pal4/cegui_ui_event_processor.h"

namespace pal4 {
namespace {

std::uint32_t RemapArrowOrEnter(const std::uint32_t key) {
    switch (key) {
        case 17u: return 200u;
        case 30u: return 203u;
        case 31u: return 208u;
        case 32u: return 205u;
        case 57u: return 28u;
        default: return key;
    }
}

void FireDisplaySizeChanged(const std::uint32_t system,
                            ICeguiUiEventRuntime& runtime) {
    const auto renderer = runtime.ReadDword(system + 0x1cu);
    if (renderer == 0u) return;
    runtime.RendererReadDisplaySize(renderer);
    runtime.RendererFireDisplaySizeChanged(renderer);
}

}  // namespace

std::uint32_t CeguiUiEventRuntimeStub::MapVirtualKey(
    std::uint32_t /*owner*/, const std::uint32_t virtual_key) {
    return virtual_key;
}
std::uint32_t CeguiUiEventRuntimeStub::MapImeCharacter(
    const std::uint32_t wparam) { return wparam; }
std::uint32_t CeguiUiEventRuntimeStub::GetSystemSingleton() { return 0u; }
std::uint32_t CeguiUiEventRuntimeStub::GetSystemSingletonPtr() { return 0u; }
std::uint32_t CeguiUiEventRuntimeStub::ReadDword(
    std::uint32_t /*address*/) { return 0u; }
bool CeguiUiEventRuntimeStub::InjectKeyDown(
    std::uint32_t /*system*/, std::uint32_t /*key*/) { return false; }
bool CeguiUiEventRuntimeStub::InjectKeyUp(
    std::uint32_t /*system*/, std::uint32_t /*key*/) { return false; }
bool CeguiUiEventRuntimeStub::InjectChar(
    std::uint32_t /*system*/, std::uint32_t /*character*/) { return false; }
void CeguiUiEventRuntimeStub::EnableMouseCapture(
    std::uint32_t /*owner*/) {}
void CeguiUiEventRuntimeStub::DisableMouseCapture(
    std::uint32_t /*owner*/) {}
std::array<float, 2> CeguiUiEventRuntimeStub::TransformMouseCoordinates(
    std::uint32_t /*owner*/, const std::array<float, 2>& point) {
    return point;
}
bool CeguiUiEventRuntimeStub::InjectMousePosition(
    std::uint32_t /*system*/, float /*x*/, float /*y*/) { return false; }
bool CeguiUiEventRuntimeStub::InjectMouseButtonDown(
    std::uint32_t /*system*/, std::uint32_t /*button*/) { return false; }
bool CeguiUiEventRuntimeStub::InjectMouseButtonUp(
    std::uint32_t /*system*/, std::uint32_t /*button*/) { return false; }
bool CeguiUiEventRuntimeStub::InjectMouseWheel(
    std::uint32_t /*system*/, float /*delta*/) { return false; }
void CeguiUiEventRuntimeStub::RendererReadDisplaySize(
    std::uint32_t /*renderer*/) {}
void CeguiUiEventRuntimeStub::RendererFireDisplaySizeChanged(
    std::uint32_t /*renderer*/) {}
void CeguiUiEventRuntimeStub::RequestRedraw(
    std::uint32_t /*window*/) {}

bool CeguiUiEventProcessor::Process(
    const std::uint32_t owner, const std::uint32_t /*hwnd*/,
    const std::uint32_t message, const std::uint32_t wparam,
    const std::uint32_t lparam, ICeguiUiEventRuntime& runtime) {
    switch (message) {
        case 5u: {
            if (runtime.GetSystemSingletonPtr() != 0u) {
                FireDisplaySizeChanged(runtime.GetSystemSingleton(), runtime);
            }
            return false;
        }
        case 6u: {
            if (wparam != 0u && runtime.GetSystemSingletonPtr() != 0u) {
                FireDisplaySizeChanged(runtime.GetSystemSingleton(), runtime);
                const auto system = runtime.GetSystemSingletonPtr();
                runtime.RequestRedraw(runtime.ReadDword(system + 0x30u));
            }
            return false;
        }
        case 0xa0u:
        case 0x2a3u:
            runtime.DisableMouseCapture(owner);
            return false;
        case 0x100u: {
            const auto key = runtime.MapVirtualKey(owner, wparam);
            if (key == 1u) return false;
            return runtime.InjectKeyDown(runtime.GetSystemSingleton(),
                                         RemapArrowOrEnter(key));
        }
        case 0x101u: {
            const auto key = runtime.MapVirtualKey(owner, wparam);
            if (key == 1u) {
                static_cast<void>(runtime.InjectKeyDown(
                    runtime.GetSystemSingleton(), key));
            }
            return runtime.InjectKeyUp(runtime.GetSystemSingleton(),
                                       RemapArrowOrEnter(key));
        }
        case 0x102u:
            return runtime.InjectChar(runtime.GetSystemSingleton(), wparam);
        case 0x200u: {
            runtime.EnableMouseCapture(owner);
            const std::array<float, 2> point{
                static_cast<float>(lparam & 0xffffu),
                static_cast<float>((lparam >> 16u) & 0xffffu)};
            const auto transformed =
                runtime.TransformMouseCoordinates(owner, point);
            return runtime.InjectMousePosition(runtime.GetSystemSingleton(),
                                               transformed[0], transformed[1]);
        }
        case 0x201u:
            return runtime.InjectMouseButtonDown(
                runtime.GetSystemSingleton(), 0u);
        case 0x202u:
            return runtime.InjectMouseButtonUp(
                runtime.GetSystemSingleton(), 0u);
        case 0x204u:
            return runtime.InjectMouseButtonDown(
                runtime.GetSystemSingleton(), 1u);
        case 0x205u:
            return runtime.InjectMouseButtonUp(
                runtime.GetSystemSingleton(), 1u);
        case 0x207u:
            return runtime.InjectMouseButtonDown(
                runtime.GetSystemSingleton(), 2u);
        case 0x208u:
            return runtime.InjectMouseButtonUp(
                runtime.GetSystemSingleton(), 2u);
        case 0x20au: {
            const auto signed_delta = static_cast<std::int16_t>(
                (wparam >> 16u) & 0xffffu);
            const auto wheel = static_cast<float>(signed_delta) *
                               0.0083333338f;
            return runtime.InjectMouseWheel(
                runtime.GetSystemSingleton(), wheel);
        }
        case 0x286u: {
            const auto character = runtime.MapImeCharacter(wparam);
            return runtime.InjectChar(
                runtime.GetSystemSingleton(), character);
        }
        default:
            return false;
    }
}

}  // namespace pal4
