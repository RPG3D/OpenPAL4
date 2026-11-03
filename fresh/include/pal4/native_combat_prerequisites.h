#pragma once
#include "pal4/native_matfx_script_session.h"
#include <cstdint>
#include <memory>
#include <string_view>

struct IDirect3DDevice9;
namespace pal4 {
class NativePkFileManagerRuntime;
class NativeCameraManagerOwner;

// IDA53F870: uses the existing camera owner and shared CombatCameraAccess.
// Updates CombatCamera only; does not activate it or replace the Scene camera.
[[nodiscard]] std::uint32_t SetupNativeCombatCamera(NativeCameraManagerOwner&);
}
