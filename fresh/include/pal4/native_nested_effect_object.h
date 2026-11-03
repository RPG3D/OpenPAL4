#pragma once
#include "pal4/effect_object.h"
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeNestedEffectDialogs;
class INativeDialogParticleQueues;
class IEffectProximityRuntime;

class NativeNestedEffectObject final {
public:
    NativeNestedEffectObject(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        std::uint32_t system,NativeNestedEffectDialogs&,INativeDialogParticleQueues&);
    ~NativeNestedEffectObject();
    EffectObjectState& State();
    bool Load(std::uint32_t document);
    std::uint32_t Frame();
    void SetMode(std::uint32_t mode);
    bool Reset();
    void Start(std::uint8_t loop);
    void ClearPlayback();
    void PausePlayback();
    void ResumePlayback();
    float RemainingPlaybackTime();
    void Advance(double seconds);
    void Draw(std::uint32_t context,std::uint32_t gate,IEffectProximityRuntime&);
    void ReadScale(std::array<float,3>&);
    void ReadPosition(std::array<float,3>&);
    void ReadRotation(std::array<float,3>&);
    void Rotate(const std::array<float,3>&,bool relative);
    void Move(const std::array<float,3>&,bool relative);
    void SetScale(const std::array<float,3>&,std::uint32_t mode);
    void SetAimTarget(const std::array<float,3>&);
    void SetAimRotation(const std::array<float,3>&);
    void ActivateAim();
    void ReleasePlayback();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
} // namespace pal4
