#pragma once
#include "pal4/strip_object.h"
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextureDictionary;
class IEffectProximityRuntime;
class INativeDialogParticleQueues;
// The 300-byte StripObject owner; StripSystemEffect weapon trails are separate.
class NativeStripObject final {
public:
    NativeStripObject(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
                     NativeEffectTextureDictionary&,std::uint32_t system);
    ~NativeStripObject();
    StripObjectState& State();
    bool Load(std::uint32_t document);
    std::uint32_t Frame();
    void Show(bool);
    void SetMode(std::uint32_t);
    bool Reset();
    void Advance(double);
    std::uint8_t Draw(std::uint32_t context,std::uint32_t gate,IEffectProximityRuntime&);
    void ReadScale(std::array<float,3>&);
    void ReadRotation(std::array<float,3>&);
    void ReadPosition(std::array<float,3>&);
    void SetScale(const std::array<float,3>&,std::uint32_t mode);
    void Rotate(const std::array<float,3>&,bool relative);
    void Move(const std::array<float,3>&,bool relative);
    void ReadColor(std::array<std::uint8_t,4>&);
    void SetColor(const std::array<std::uint8_t,4>&);
    void ReleasePlayback(INativeDialogParticleQueues&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
