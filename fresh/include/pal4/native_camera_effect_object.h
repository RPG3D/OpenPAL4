#pragma once
#include "pal4/effect_camera_object.h"
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectDialogTargets;
class INativeDialogParticleQueues;
// Owns the effect source frame, camera snapshot and cached XML. Borrows queue
// routing and the context group; stop before the context camera is retired.
class NativeCameraEffectObject final {
public:
    NativeCameraEffectObject(NativeBaseObjectMemory&, NativeEventDocumentRuntime&,
                             std::uint32_t system, INativeDialogParticleQueues&,NativeEffectDialogTargets* targets=nullptr);
    ~NativeCameraEffectObject();
    EffectCameraObjectState& State();
    bool Load(std::uint32_t);
    bool Reset();
    std::uint32_t Frame();
    void SetMode(std::uint32_t);
    void Start();
    void Pause();
    void Advance();
    void ReleasePlayback();
    void ReadPosition(std::array<float,3>&);
    void Move(const std::array<float,3>&,bool);
    void ReadRotation(std::array<float,3>&);
    void Rotate(const std::array<float,3>&,bool);
    void SetTrackPosition(const std::array<float,3>&);
    void SetTrackDirection(const std::array<float,3>&);
    void SetLockedLookAt(bool);
    bool LockedLookAt();
    void Revolve(const std::array<float,3>& pivot,const std::array<float,3>& rotation,const std::array<float,3>& step);
    bool Ready();
    bool InheritCurrent();
    void ReadTarget(std::array<float,3>&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
