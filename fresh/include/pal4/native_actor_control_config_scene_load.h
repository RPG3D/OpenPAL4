#pragma once

#include <cstdint>

namespace pal4 {
class NativeActorControlSession;
class NativeCameraManagerOwner;
class NativeMinimapManagerOwner;
class NativeScenePlayerMovement;
class NativeFloorAttributeOwner;
class NativeGameSaveArchiveReader;
class INativeScriptCameraListener;

class INativeActorControlConfigHost {
public:
    virtual ~INativeActorControlConfigHost() = default;
    virtual void ProcessActorInput(bool clear_vector) = 0;
    virtual void ResetLoadedActorControl() = 0;
};

class NativeActorControlConfigSceneLoad final {
public:
    // Borrows the current initialized scene. Input/reset use the same host
    // services as scene transitions and player lock/unlock.
    [[nodiscard]] static std::uint8_t Deserialize(
        NativeActorControlSession& control, NativeCameraManagerOwner& cameras,
        NativeMinimapManagerOwner& minimap, NativeScenePlayerMovement& movement,
        NativeFloorAttributeOwner& floor, INativeScriptCameraListener& listener,
        INativeActorControlConfigHost& host, NativeGameSaveArchiveReader& archive);
};
}  // namespace pal4
