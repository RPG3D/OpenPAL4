#pragma once
#include "pal4/particle_visual_object.h"

namespace pal4 {
class NativeBaseObjectMemory;
// Reads the actual borrowed emitter atomic and uses public SDK skin/matrix APIs.
// Source clump, hierarchy and engine must outlive particle updates.
class NativeParticleSkinRuntime final : public IParticleSkinRuntime {
public:
    explicit NativeParticleSkinRuntime(NativeBaseObjectMemory& memory) : memory_(memory) {}
    std::uint32_t SourceGeometry(std::uint32_t source) override;
    std::uint32_t Skin(std::uint32_t geometry) override;
    int MorphTargetCount(std::uint32_t geometry) override;
    std::uint32_t VertexCount(std::uint32_t geometry) override;
    std::uint32_t Hierarchy(std::uint32_t source) override;
    ParticleVisualPoint GeometryVertex(std::uint32_t geometry,std::uint32_t vertex) override;
    std::array<std::uint8_t,4> BoneIndices(std::uint32_t skin,std::uint32_t vertex) override;
    std::array<float,4> BoneWeights(std::uint32_t skin,std::uint32_t vertex) override;
    ParticleVisualPoint TransformBoneVertex(std::uint32_t skin,std::uint32_t hierarchy,
        std::uint8_t bone,ParticleVisualPoint vertex) override;
    ParticleVisualPoint TransformFrameVertex(std::uint32_t frame,ParticleVisualPoint vertex) override;
    std::uint32_t AllocatePointBuffer(std::uint32_t bytes) override;
    ParticleVisualPoint ReadPointBuffer(std::uint32_t address) override;
    void WritePointBuffer(std::uint32_t address,ParticleVisualPoint value) override;
    void FreePointBuffer(std::uint32_t address) override;
private:
    NativeBaseObjectMemory& memory_;
};
}
