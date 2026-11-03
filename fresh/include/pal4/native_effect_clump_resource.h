#pragma once
#include "pal4/game_resource_manager.h"
#include <filesystem>
#include <memory>

namespace pal4 {
class NativePkFileManagerRuntime;

// Owns a model-bearing effect resource. Package services are borrowed for
// material metadata and must outlive this owner and any loaded material data.
class NativeEffectClumpResource final {
public:
    NativeEffectClumpResource(const std::filesystem::path&,NativePkFileManagerRuntime&);
    ~NativeEffectClumpResource();
    bool Load(std::string_view name,std::string_view directory);
    void Unload();
    void Update(float delta_time);
    GameResourceManager& State();
    void Rotate(const std::array<float,3>&,bool relative);
    void Scale(const std::array<float,3>&,bool relative);
    void Translate(const std::array<float,3>&,bool relative);
    void SetColor(std::uint32_t rgba);
    void CleanupTextures();
    void ClearTexture();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
