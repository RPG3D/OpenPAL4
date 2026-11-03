#pragma once
#include "pal4/portable/scene.h"
#include <string>
#include <optional>
#include <functional>
#include <memory>

namespace pal4::portable {
using Matrix = std::array<float,16>;
struct ActorFrame { std::int32_t parent; Matrix local; };
struct SkinPalette { std::uint32_t frame; Matrix inverse_bind; };
struct SkinInfluence { std::array<std::uint32_t,4> bones; std::array<float,4> weights; };
struct AnimationKey { float time; std::array<float,4> rotation; std::array<float,3> translation; };
struct AnimationTrack { std::uint32_t frame; std::vector<AnimationKey> keys; };
struct AnimationClip { std::string name; float duration; std::vector<AnimationTrack> tracks; };
struct ActorAtomicSphere {std::uint32_t frame{};std::array<float,4> sphere{};};
struct ActorAtomicBinding {std::uint32_t geometry{},flags{};};
struct ActorDrawBinding {std::uint32_t atomic{},material_slot{};};
struct ActorRenderBindings {
    std::vector<std::array<std::uint8_t,4>> materials;
    std::vector<std::vector<std::uint32_t>> geometry_materials;
    std::vector<ActorAtomicBinding> atomics;
    std::vector<ActorDrawBinding> draws;
};
struct ActorLocalBox {std::array<float,3> maximum{},minimum{};};
using ActorGeometryVertices=std::vector<std::array<float,3>>;
struct Actor {
    Scene mesh;
    std::vector<ActorFrame> frames;
    std::vector<SkinPalette> palettes;
    std::vector<SkinInfluence> influences;
    std::vector<AnimationClip> clips;
    std::function<std::shared_ptr<const AnimationClip>(std::string_view)> clip_loader;
    std::vector<ActorAtomicSphere> atomic_spheres;
    // v1/v2 have no render mapping. Never infer it from draw counts or spheres.
    std::optional<ActorRenderBindings> render_bindings;
    // v4保留所有原geometry顶点，含隐藏/非绘制atomic；不以绘制三角形近似包围盒。
    std::optional<std::vector<ActorGeometryVertices>> bind_vertices;
};
// Original bind-frame bounds, merged by the shared 667C60 algorithm.
// Version 1 caches lack this metadata and cannot provide a movement radius.
std::optional<std::array<float,4>> ActorBindSphere(const Actor& actor,bool direct=false);
std::optional<ActorLocalBox> ActorBindBox(const Actor& actor,bool direct=false);
// P4AR is an offline resource transport. It contains no native pointers or
// platform data. Clip -1 samples the DFF bind hierarchy, not an invented pose.
Actor ReadActor(std::istream& stream);
// GameResourceManager::UpdateClumpTransform (666550) replaces the clump root
// with the scene pose. With an external instance matrix, its local root is identity.
// Call at instance setup, never while parsing a reusable original DFF resource.
void PrepareActorInstanceRoot(Actor& actor);
// GameResourceManager_LoadClump 665ECE applies material mode 1 (linear),
// overriding the DFF filter. Keep this at the resource boundary, not the parser.
void PrepareActorTextureFiltering(Actor& actor);
enum class AnimationTimeMode {loop,clamp};
// Optional frame output uses the same sampled pose as vertices, including blends.
// Matrices are relative to the actor root's parent, before the instance transform.
void SampleActorVertices(const Actor& actor, int clip, float seconds, std::vector<Vertex>& output,AnimationTimeMode mode=AnimationTimeMode::loop,std::vector<Matrix>* frames=nullptr);
void SampleBlendedActorVertices(const Actor& actor,int from,int to,float weight,std::vector<Vertex>& output,std::vector<Matrix>* frames=nullptr);
// 动作可由播放器独立持有；空指针表示绑定姿势。采样期间调用方须保持动作存活。
void SampleActorClipVertices(const Actor& actor,const AnimationClip* clip,float seconds,std::vector<Vertex>& output,AnimationTimeMode mode=AnimationTimeMode::loop,std::vector<Matrix>* frames=nullptr);
void SampleBlendedActorClipVertices(const Actor& actor,const AnimationClip& from,const AnimationClip& to,float weight,std::vector<Vertex>& output,std::vector<Matrix>* frames=nullptr);
class ActorClipLease {
public:
    void Sample(const Actor& actor,int clip,float seconds,std::vector<Vertex>& output,AnimationTimeMode mode=AnimationTimeMode::loop,std::vector<Matrix>* frames=nullptr);
    void Reset() noexcept;
private:
    const Actor* owner_{};
    int index_{-1};
    std::shared_ptr<const AnimationClip> clip_;
};
}  // namespace pal4::portable
