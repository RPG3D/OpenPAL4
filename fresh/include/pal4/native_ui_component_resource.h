#pragma once
#include "pal4/base_object_lifecycle.h"
#include <array>
#include <filesystem>
#include <memory>
#include <string_view>
struct RpClump;
namespace pal4 {
class NativePkFileManagerRuntime;
class NativePalGameIVTextureStorage;
// Owns one original UI component's prepared clump/UV/config/texture resources.
// The package/game owners and RenderWare engine must outlive this owner.
class NativeUiComponentResource final {
public:
    NativeUiComponentResource(const std::filesystem::path&,NativePkFileManagerRuntime&,
        NativePalGameIVTextureStorage&);
    ~NativeUiComponentResource();
    NativeUiComponentResource(const NativeUiComponentResource&)=delete;
    NativeUiComponentResource& operator=(const NativeUiComponentResource&)=delete;
    bool Load(std::uint32_t id,std::string_view prefix,std::string_view model,std::uint8_t mode=1u);
    void SetTexture(std::string_view);
    void SetBaseRotation(const std::array<float,3>&,bool relative=false);
    void SetPosition(const std::array<float,3>&);
    void SetScale(const std::array<float,3>&);
    void SetRotationRates(bool active,const std::array<float,3>&);
    void Advance(float elapsed);
    void AddToWorld(std::uint32_t world);
    void RemoveFromWorld();
    bool Render(std::uint32_t camera);
    void Clear();
    std::uint32_t Address() const noexcept;
    const BaseObjectState& State() const noexcept;
    RpClump* Clump() const noexcept;
    std::size_t OutstandingAllocations() const noexcept;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
