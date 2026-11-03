#pragma once
#include "pal4/sequence_anim_object.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextureDictionary;
class IEffectProximityRuntime;
class INativeDialogParticleQueues;

// Owns the sequence's visual/frame/plane. Atlas textures belong to the shared
// effect dictionary; destroy this owner before the dictionary and SDK shutdown.
class NativeSequenceAnimObject final {
public:
    NativeSequenceAnimObject(NativeBaseObjectMemory&, NativeEventDocumentRuntime&,
        NativeEffectTextureDictionary&, std::uint32_t system);
    ~NativeSequenceAnimObject();
    SequenceAnimObjectState& State();
    bool Load(std::uint32_t document);
    std::uint32_t Frame();
    void Show(bool);
    void SetMode(std::uint32_t);
    bool Reset();
    void Advance(double);
    std::uint8_t Draw(std::uint32_t context, std::uint32_t gate, IEffectProximityRuntime&);
    void ReadScale(std::array<float,3>&);
    void ReadPosition(std::array<float,3>&);
    void ReadRotation(std::array<float,3>&);
    void Rotate(const std::array<float,3>&, bool relative);
    void Move(const std::array<float,3>&, bool relative);
    void SetTrackPosition(const std::array<float,3>&);
    void SetScale(const std::array<float,3>&, std::uint32_t mode);
    void ReadColor(std::array<std::uint8_t,4>&);
    void SetColor(const std::array<std::uint8_t,4>&);
    void ReleasePlayback(INativeDialogParticleQueues&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
