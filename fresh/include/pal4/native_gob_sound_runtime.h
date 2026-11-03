#pragma once
#include "pal4/native_gob_object_image.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeCameraManagerOwner;
class NativeMilesSound3dRuntime;
struct NativeGobSoundServices {
    NativeCameraManagerOwner* cameras{};
    std::function<NativeMilesSound3dRuntime&()> sounds;
};
// Borrows the scene image/config/memory and lazily opened real audio owner.
// Destroy before those owners and before their creating SDK resources retire.
class NativeGobSoundRuntime final : public INativeGobSoundService {
public:
    NativeGobSoundRuntime(NativeGobObjectImage&,const NativeSceneGobRecord&,
        NativeBaseObjectMemory&,NativeGobSoundServices);
    ~NativeGobSoundRuntime();
    void Reload(const NativeSceneGobRecord&) override;
    void SetActive(bool) override;
    std::uint32_t StopAndRemove() override;
    void Advance(float elapsed);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
