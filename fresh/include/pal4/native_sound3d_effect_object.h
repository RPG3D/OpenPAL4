#pragma once
#include "pal4/sound3d_object.h"
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class INativeEffectSound;
class INativeDialogParticleQueues;
class NativeSound3dEffectObject final {
public:
    NativeSound3dEffectObject(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        std::uint32_t system,INativeEffectSound*);
    ~NativeSound3dEffectObject();
    Sound3DObjectState& State();
    bool Load(std::uint32_t document);
    std::uint32_t Frame();
    void SetMode(std::uint32_t);
    bool Reset();
    void Advance();
    void Play(std::uint8_t once);
    void Stop();
    void ReadPosition(std::array<float,3>&);
    void Move(const std::array<float,3>&,bool relative);
    void ReleasePlayback(INativeDialogParticleQueues&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
