#pragma once
#include "pal4/dynamic_strip_object.h"
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextureDictionary;
class IEffectProximityRuntime;
class INativeDialogParticleQueues;
// Owns the228-byte DynamicStrip and its136-byte visual at+52.
class NativeDynamicStripObject final {
public:
    NativeDynamicStripObject(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        NativeEffectTextureDictionary&,std::uint32_t system);
    ~NativeDynamicStripObject();
    DynamicStripObjectState& State();
    bool Load(std::uint32_t);
    std::uint32_t Frame();
    void Show(bool);
    void SetMode(std::uint32_t);
    bool Reset();
    void Advance(double);
    std::uint8_t Draw(std::uint32_t,std::uint32_t,IEffectProximityRuntime&);
    void ReadScale(std::array<float,3>&);
    void ReadRotation(std::array<float,3>&);
    void ReadPosition(std::array<float,3>&);
    void SetScale(const std::array<float,3>&,std::uint32_t);
    void Rotate(const std::array<float,3>&,bool);
    void Move(const std::array<float,3>&,bool);
    void SetTrackPosition(const std::array<float,3>&);
    void ReadColor(std::array<std::uint8_t,4>&);
    void SetColor(const std::array<std::uint8_t,4>&);
    void ReleasePlayback(INativeDialogParticleQueues&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
