#pragma once
#include "pal4/thunder_object.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextureDictionary;
class NativeEffectDialogTargets;
class INativeDialogParticleQueues;
class IEffectProximityRuntime;

// Owns the original ThunderObject image, frames, texture reference and path.
// Dialog targets are borrowed until this object is retired.
class NativeThunderEffectObject final {
public:
    NativeThunderEffectObject(NativeBaseObjectMemory&, NativeEventDocumentRuntime&,
        NativeEffectTextureDictionary&, std::uint32_t system, NativeEffectDialogTargets*);
    ~NativeThunderEffectObject();
    ThunderObjectState& State();
    bool Load(std::uint32_t document);
    std::uint32_t Frame();
    void Show(bool visible);
    void SetMode(std::uint32_t mode);
    bool Reset();
    void Advance(double elapsed);
    std::uint8_t Draw(std::uint32_t context, std::uint32_t gate, IEffectProximityRuntime&);
    void ReleasePlayback(INativeDialogParticleQueues&);
    void ReadScale(std::array<float, 3>&);
    void ReadPosition(std::array<float, 3>&);
    void ReadRotation(std::array<float, 3>&);
    void SetScale(const std::array<float, 3>&, std::uint32_t mode);
    void Move(const std::array<float, 3>&, bool relative);
    void Rotate(const std::array<float, 3>&, bool relative);
    void ReadColor(std::array<std::uint8_t, 4>&);
    void SetColor(const std::array<std::uint8_t, 4>&);
    void SetTrackPosition(const std::array<float, 3>&);
    void Revolve(const std::array<float, 3>& pivot, const std::array<float, 6>&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
