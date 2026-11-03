#pragma once
#include <array>
#include <cstdint>
#include <memory>
struct RpWorld;
struct RwTexture;
namespace pal4 {
struct NativeWorldSectorRenderStats {
    unsigned sectors{},draws{},primitives{},effect_passes{},dual_meshes{},alpha_queries{};
};
// Calls the exact matched SDK alpha query (76B260); null textures are invalid.
[[nodiscard]] bool NativeWorldTextureHasAlpha(RwTexture* texture);
// Owns the common Scene51D630/Combat53E4C0 pipeline bindings. Pins the
// compiled effect until sector resources and callbacks have been retired.
class NativeWorldSectorRenderer final {
public:
    NativeWorldSectorRenderer(RpWorld& world,std::uint32_t effect,std::array<float,4> lightmap);
    // Scene51D6C0 reads live color parameters from arena+428..440.
    NativeWorldSectorRenderer(RpWorld& world, std::uint32_t effect, std::uint32_t arena);
    ~NativeWorldSectorRenderer();
    // Called after SDK world destruction, before the original pipeline tail.
    void WorldReleased() noexcept;
    NativeWorldSectorRenderer(const NativeWorldSectorRenderer&)=delete;
    NativeWorldSectorRenderer& operator=(const NativeWorldSectorRenderer&)=delete;
    // Caller begins the world camera and sets the enclosing game render state.
    void Render();
    [[nodiscard]] NativeWorldSectorRenderStats Stats() const noexcept;
    [[nodiscard]] std::uint32_t PipelineAddress() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
