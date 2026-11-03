#pragma once
#include "pal4/portable/scene.h"
#include <GLES3/gl3.h>
#include <span>

namespace pal4::portable {
class EffectCylinder;
// Winding in OpenGL clip/NDC coordinates, before the display's Y inversion.
enum class FaceCulling {none,clockwise,counter_clockwise};
enum class EffectBlendFactor {zero,one,source_color,inverse_source_color,source_alpha,inverse_source_alpha,destination_color,inverse_destination_color,destination_alpha,inverse_destination_alpha};
struct EffectRenderState {
    EffectBlendFactor source{EffectBlendFactor::source_alpha},destination{EffectBlendFactor::inverse_source_alpha};
    bool blend{true},depth_test{true},depth_write{};
};
class GlesRenderer {
public:
    void Upload(const Scene& scene);
    void UpdateVertices(const std::vector<Vertex>& vertices);
    // Refresh a dynamic image without recreating the shader program or mesh.
    void UpdateTexture(std::size_t index,const Texture& texture);
    void Render(const Scene& scene, const OrbitCamera& camera, int width, int height);
    static void BeginFrame(int width, int height);
    // Multiple resources share one frame/depth buffer. Model transforms never
    // mutate immutable CPU meshes or force the world buffer to be re-uploaded.
    // Without an appearance override, lightmapped draws follow PAL4 ltMap.fx
    // (prelight, diffuse and lightmap; no stored material colour). Other draws
    // retain their material colour. Explicit appearance supplies a colour tint.
    std::uint32_t DrawScene(const Scene& scene, const std::array<float,16>& view_projection,
                   const std::array<float,16>& model,std::span<const DrawAppearance> appearance={},FaceCulling culling=FaceCulling::none,float alpha_cutoff=.1f);
    std::uint32_t DrawEffect(const Scene& scene,const std::array<float,16>& view_projection,
                   const std::array<float,16>& model,const EffectRenderState& state,std::span<const DrawAppearance> appearance={});
    std::uint32_t DrawSky(const Scene&,const std::array<float,16>& view_projection);
    std::uint32_t DrawCylinder(const EffectCylinder& cylinder,const std::array<float,16>& view_projection);
    void Release();
    // A lost EGL context owns and retires its old objects. Never delete old
    // names against a newly created context, where names can be reused.
    void Abandon();
private:
    std::uint32_t DrawConfigured(const Scene&,const std::array<float,16>&,const std::array<float,16>&,
                   std::span<const DrawAppearance>,const EffectRenderState*,bool sky=false,FaceCulling culling=FaceCulling::none,float alpha_cutoff=.1f);
    GLuint program_{}, buffer_{}, vao_{}, white_{};
    std::vector<GLuint> textures_;
    GLint sky_{},matrix_{}, color_{}, saturation_{}, brightness_{}, alpha_cutoff_{};
    std::array<GLint,2> uv_transform_{};
    std::size_t vertex_count_{};
};
// Runs against the current GLES context and checks intersecting world surfaces.
// It catches transparent world passes that accidentally disable depth writes.
std::array<std::uint8_t,4> VerifyWorldDepthContract();
void VerifySceneCompositionContract();
void VerifyEffectGeometryContract();
}  // namespace pal4::portable
