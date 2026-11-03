#pragma once

#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace pal4 {

enum class WorldSectorRenderCall : std::uint8_t {
    CreatePipeline,
    GetNodeDefinition,
    SetMipmapping,
    LockPipeline,
    AddFragment,
    UnlockPipeline,
    FindNodeByName,
    SetInstanceCallback,
    SetRenderCallback,
    DestroyPipeline,
    SetPixelShader,
    EnableClipping,
    SetIndices,
    SetStreams,
    SetVertexDeclaration,
    GetRenderState,
    SetRenderState,
    SetVertexAlpha,
    SetTextureStageState,
    SetTextureStage,
    GetVertexAlphaPreference,
    GetMaterialEffects,
    GetDualTexture,
    SetVertexShader,
    DrawIndexed,
    DrawPrimitive,
    TextureHasAlpha,
    GetEffectVm,
    GetArenaManager,
    VmSetVector,
    VmSetFloat,
    VmFindTechnique,
    VmPassCount,
    VmSetPass,
    VmEndPass,
    VmEndTechnique,
};

class IWorldSectorRenderRuntime {
public:
    virtual ~IWorldSectorRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        WorldSectorRenderCall operation,
        std::initializer_list<std::uint32_t> arguments = {},
        std::string_view text = {}) = 0;
};

class WorldSectorLightmapPipeline final {
public:
    // IDA 0x51d630.
    [[nodiscard]] static std::uint32_t Create(IWorldSectorRenderRuntime& runtime);
    // IDA 0x51d6c0. render_data is the stack argument, not an IDA register guess.
    static void RenderCallback(
        std::uint32_t render_data, std::uint32_t clipping,
        std::uint32_t clipping_flags, std::uint32_t render_flags,
        IWorldSectorRenderRuntime& runtime);
    // IDA 0x51d990. header points to render_data + 24.
    static void Render(
        std::uint32_t header, bool use_alpha_texture,
        IWorldSectorRenderRuntime& runtime);
};

}  // namespace pal4
