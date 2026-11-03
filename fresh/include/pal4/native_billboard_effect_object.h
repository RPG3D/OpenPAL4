#pragma once
#include "pal4/effect_system_object_f.h"
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextureDictionary;
class IEffectProximityRuntime;
class INativeDialogParticleQueues;
class NativeBillboardEffectObject final {
public:
    NativeBillboardEffectObject(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
                              NativeEffectTextureDictionary&,std::uint32_t system);
    ~NativeBillboardEffectObject();
    EffectSystemObjectFState& State();
    bool Load(std::uint32_t document);
    std::uint32_t Frame();
    void Show(bool visible);
    void SetMode(std::uint32_t mode);
    bool Reset();
    void Advance(double delta);
    void Revolve(const std::array<float,3>&,const std::array<float,3>&,const std::array<float,3>&);
    std::uint8_t Draw(std::uint32_t context,std::uint32_t gate,IEffectProximityRuntime&);
    void ReadScale(std::array<float,3>&);
    void ReadPosition(std::array<float,3>&);
    void ReadRotation(std::array<float,3>&);
    void Rotate(const std::array<float,3>&,bool relative);
    void Move(const std::array<float,3>&,bool relative);
    void SetScale(const std::array<float,3>&,std::uint32_t mode);
    void ReadColor(std::array<std::uint8_t,4>&);
    void SetColor(const std::array<std::uint8_t,4>&);
    void ReleasePlayback(INativeDialogParticleQueues&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
} // namespace pal4
