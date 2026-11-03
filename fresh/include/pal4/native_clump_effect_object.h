#pragma once
#include "pal4/clump_object.h"
#include <memory>
#include <filesystem>
#include <functional>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativePkFileManagerRuntime;
class NativeEffectSupportObjects;
class NativeArenaEffectServices;
class INativeDialogParticleQueues;

// Owns the original ClumpObject image, XML and optional model/animation resource.
// Explicit root/package and visibility services enable model playback.
class NativeClumpEffectObject final {
public:
    NativeClumpEffectObject(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,std::uint32_t system,
        const std::filesystem::path& root={},NativePkFileManagerRuntime* packages=nullptr);
    ~NativeClumpEffectObject();
    ClumpObjectState& State();
    bool Load(std::uint32_t document);
    std::uint32_t Frame();
    void SetMode(std::uint32_t);
    bool Reset();
    // Borrowed visibility services, including their arena, must outlive this
    // object. Bind before showing a configured model.
    void BindVisibility(NativeEffectSupportObjects&,NativeArenaEffectServices&);
    // Pooled models can change Dialog owner. Resolve from the current original
    // owner field at dispatch time; the resolver and all groups outlive models.
    using GeometryResolver=std::function<NativeArenaEffectServices&(std::uint32_t)>;
    void BindVisibility(NativeEffectSupportObjects&,GeometryResolver);
    void Show(bool);
    double Duration();
    void Advance(double delta_time);
    std::uint8_t Draw(std::uint32_t context);
    void ReadColor(std::array<std::uint8_t,4>&);
    void SetColor(const std::array<std::uint8_t,4>&);
    void ReadPosition(std::array<float,3>&);
    void Move(const std::array<float,3>&,bool relative);
    void SetTrackPosition(const std::array<float,3>&);
    void ReadScale(std::array<float,3>&);
    void SetScale(const std::array<float,3>&,bool relative);
    void ReadRotation(std::array<float,3>&);
    void Rotate(const std::array<float,3>&,bool relative);
    void SetAimTarget(const std::array<float,3>&);
    void SetAimRotation(const std::array<float,3>&);
    void ActivateAim();
    void ReleasePlayback(INativeDialogParticleQueues&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
