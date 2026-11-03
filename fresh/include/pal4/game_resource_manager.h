#pragma once

#include "pal4/game_config_state.h"
#include "pal4/rw_dict_schema.h"
#include "pal4/rw_geometry_flags.h"
#include "pal4/rw_clump_optimizer.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct ResourceVec3 final { float x{}, y{}, z{}; };
using ResourceCorners = std::array<ResourceVec3, 8>;

// 原667620逐帧更新只依赖阴影场景提交；装载/渲染接口保持独立。
class IGameResourceFrameRuntime {
public:
    virtual ~IGameResourceFrameRuntime()=default;
    virtual void RenderCameraScene(std::uint32_t* camera,float elapsed,std::uint32_t* manager)=0;
};

// 主clump的变换与包围数据边界，独立于纹理/摄像机装载。
class IGameResourcePoseRuntime {
public:
    virtual ~IGameResourcePoseRuntime()=default;
    [[nodiscard]] virtual std::uint32_t ClumpFrame(std::uint32_t clump) = 0;
    virtual void FrameSetIdentity(std::uint32_t frame) = 0;
    virtual void FrameRotate(std::uint32_t frame, std::uint32_t axis_address,
                             float degrees, std::uint32_t combine) = 0;
    virtual void FrameUpdateObjects(std::uint32_t frame) = 0;
    virtual void FrameScale(std::uint32_t frame, const float* scale,
                            std::uint32_t combine) = 0;
    virtual void FrameTranslate(std::uint32_t frame, const float* position,
                                std::uint32_t combine) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(std::uint32_t frame) = 0;
    virtual void ApplyDialogTransform(std::uint32_t frame,
                                      std::uint32_t transform,
                                      std::uint32_t combine) = 0;
    virtual void TransformPoints(ResourceCorners& corners,
                                 std::uint32_t transform) = 0;
    virtual void CalculateBBox(std::uint32_t* bbox,
                               const ResourceCorners& corners) = 0;
    [[nodiscard]] virtual std::uint32_t CalculateCameraPosition(
        std::uint32_t* destination, std::uint32_t* source,
        std::uint32_t frame) = 0;
    virtual void ProcessAtomicBounds(std::uint32_t clump,
                                     std::uint32_t mode,
                                     std::uint32_t* bbox) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessAtomicBounds2(std::uint32_t clump,
                                      std::uint32_t first,
                                      std::uint32_t second,
                                      std::uint32_t* output) = 0;
};

class IGameResourceManagerRuntime : public IGameResourcePoseRuntime, public IGameResourceFrameRuntime, public IGameConfigMemory,
                                    public IRwGeometryFlagsRuntime,
                                    public IRwClumpOptimizerRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void StoreString(std::uint32_t address, std::string_view value) = 0;
    [[nodiscard]] virtual std::string ReadString(std::uint32_t address) = 0;
    virtual void CleanupCamera() = 0;
    [[nodiscard]] virtual std::uint32_t CreateLight(std::uint32_t type) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    [[nodiscard]] virtual std::uint32_t LightFrame(std::uint32_t light) = 0;
    virtual void SetLightFrame(std::uint32_t light, std::uint32_t frame) = 0;
    virtual void AddFrameChild(std::uint32_t parent, std::uint32_t child) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void DestroyLight(std::uint32_t light) = 0;
    virtual void DestroyIm3D(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ClumpWorld(std::uint32_t clump) = 0;
    virtual void RemoveClump(std::uint32_t world, std::uint32_t clump) = 0;
    virtual void DestroyClump(std::uint32_t clump) = 0;
    virtual void RemoveTextureByName(std::uint32_t texture_context,
                                     std::string_view path) = 0;
    virtual void ImageReallocateAndCopy(std::uint32_t directory) = 0;
    [[nodiscard]] virtual std::uint32_t LoadWorldClump(std::string_view path,
        std::uint32_t mode, std::uint32_t texture_context) = 0;
    [[nodiscard]] virtual std::uint32_t SetClumpMaterialFlagsRaw(
        std::uint32_t clump, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t InitCamera(
        std::uint32_t* camera, std::uint32_t context,
        std::uint32_t shadow_clump, std::uint32_t light,
        std::uint32_t* first_param, std::uint32_t* second_param) = 0;
    void RenderCameraScene(std::uint32_t* camera, float clear_value,
                                    std::uint32_t* manager) override = 0;
    [[nodiscard]] virtual bool FrustumTestSphere(std::uint32_t camera,
                                                  std::uint32_t* sphere) = 0;
    virtual void RenderClumpRaw(std::uint32_t clump) = 0;
    virtual void RenderCameraRaw(std::uint32_t* camera_state,
                                  std::uint32_t camera) = 0;
    [[nodiscard]] virtual bool CheckTexturePath(std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t FirstMaterialAtomic(std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::string TexturePath(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::string SplitFilename(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t OpenDataStream(
        std::uint32_t type, std::uint32_t mode,
        std::string_view path) = 0;
    virtual void ReadStreamWord(std::uint32_t stream,
                                std::uint32_t* destination) = 0;
    virtual void ReadStreamWords(std::uint32_t stream,
                                 std::uint32_t* destination,
                                 std::uint32_t count) = 0;
    virtual void ReadStreamBuffer(std::uint32_t stream,
                                  std::uint32_t destination,
                                  std::uint32_t size) = 0;
    virtual void TerminateBuffer(std::uint32_t address,
                                 std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CloseDataStream(
        std::uint32_t stream) = 0;
    virtual void AddAndProcessScript8() = 0;
    virtual void CleanupAllScripts() = 0;
    [[nodiscard]] virtual std::uint32_t SetClumpMaterialFlagsForAllAtomics(
        std::uint32_t clump, std::uint32_t mode) = 0;
    virtual void PrepareTextureErrorString() = 0;
    [[nodiscard]] virtual std::uint32_t ReadTexture(std::string_view name) = 0;
    virtual void SetTextureBytes(std::uint32_t texture,
                                 std::uint8_t first,
                                 std::uint8_t second) = 0;
    virtual void ApplyMaterialPipeline(std::uint32_t clump,
        std::array<std::uint32_t, 3>& arguments) = 0;
    virtual void ApplyMaterialPipeline2(std::uint32_t clump,
        std::array<std::uint32_t, 8>& arguments) = 0;
    virtual void ApplyMaterialPipeline4(std::uint32_t clump,
        std::array<std::uint32_t, 3>& arguments) = 0;
    virtual void ApplyMaterialPipeline5(std::uint32_t clump,
        std::array<std::uint32_t, 6>& arguments) = 0;
    virtual void ApplyMaterialPipeline6(std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t CreateIm3D() = 0;
    [[nodiscard]] virtual std::uint32_t CheckTexturePathTarget(
        std::uint32_t clump, std::uint32_t target) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void ApplyMaterialPipeline3(std::uint32_t clump,
                                        std::uint32_t middle,
                                        std::uint32_t texture) = 0;
};

class GameResourceManagerRuntimeStub : public IGameResourceManagerRuntime {
public:
    std::uint32_t Free(std::uint32_t) override;
    std::uint32_t Allocate(std::uint32_t) override;
    void StoreString(std::uint32_t, std::string_view) override;
    std::string ReadString(std::uint32_t) override;
    void CleanupCamera() override;
    std::uint32_t CreateLight(std::uint32_t) override;
    std::uint32_t CreateFrame() override;
    std::uint32_t LightFrame(std::uint32_t) override;
    void SetLightFrame(std::uint32_t, std::uint32_t) override;
    void AddFrameChild(std::uint32_t, std::uint32_t) override;
    void DestroyFrame(std::uint32_t) override;
    void DestroyLight(std::uint32_t) override;
    void DestroyIm3D(std::uint32_t) override;
    std::uint32_t ClumpWorld(std::uint32_t) override;
    void RemoveClump(std::uint32_t, std::uint32_t) override;
    void DestroyClump(std::uint32_t) override;
    void RemoveTextureByName(std::uint32_t, std::string_view) override;
    void ImageReallocateAndCopy(std::uint32_t) override;
    std::uint32_t LoadWorldClump(std::string_view, std::uint32_t,
                                 std::uint32_t) override;
    std::uint32_t SetClumpMaterialFlagsRaw(std::uint32_t,
                                           std::uint32_t) override;
    std::uint32_t InitCamera(std::uint32_t*, std::uint32_t, std::uint32_t,
                             std::uint32_t, std::uint32_t*,
                             std::uint32_t*) override;
    void RenderCameraScene(std::uint32_t*, float,
                           std::uint32_t*) override;
    bool FrustumTestSphere(std::uint32_t, std::uint32_t*) override;
    void RenderClumpRaw(std::uint32_t) override;
    void RenderCameraRaw(std::uint32_t*, std::uint32_t) override;
    std::uint32_t ReadGeometryWord(std::uint32_t) override;
    void WriteGeometryWord(std::uint32_t, std::uint32_t) override;
    std::uint32_t ForAllGeometryFlagAtomics(std::uint32_t,
        std::uint32_t (*)(std::uint32_t, std::uint32_t,
                           IRwGeometryFlagsRuntime&),
        std::uint32_t) override;
    std::uint32_t ReadOptimizerWord(std::uint32_t) override;
    void WriteOptimizerWord(std::uint32_t, std::uint32_t) override;
    void SetTriStripMethod(std::uint32_t) override;
    std::uint8_t TriStripMeshFlag() override;
    std::uint32_t ForAllOptimizerMeshes(std::uint32_t,
        std::uint32_t (*)(std::uint32_t, std::uint32_t,
                           std::uint32_t, IRwClumpOptimizerRuntime&),
        std::uint32_t) override;
    void OptimizeAtomicRaw(std::uint32_t) override;
    void OptimizeClumpRaw(std::uint32_t, std::uint32_t) override;
    std::uint32_t ForAllOptimizerAtomics(std::uint32_t,
        std::uint32_t (*)(std::uint32_t, std::uint32_t,
                           IRwClumpOptimizerRuntime&),
        std::uint32_t) override;
    bool CheckTexturePath(std::uint32_t) override;
    std::uint32_t FirstMaterialAtomic(std::uint32_t) override;
    std::string TexturePath(std::uint32_t) override;
    std::string SplitFilename(std::string_view) override;
    std::uint32_t OpenDataStream(std::uint32_t, std::uint32_t,
                                 std::string_view) override;
    void ReadStreamWord(std::uint32_t, std::uint32_t*) override;
    void ReadStreamWords(std::uint32_t, std::uint32_t*,
                         std::uint32_t) override;
    void ReadStreamBuffer(std::uint32_t, std::uint32_t,
                          std::uint32_t) override;
    void TerminateBuffer(std::uint32_t, std::uint32_t) override;
    std::uint32_t CloseDataStream(std::uint32_t) override;
    void AddAndProcessScript8() override;
    void CleanupAllScripts() override;
    std::uint32_t ClumpFrame(std::uint32_t) override;
    void FrameSetIdentity(std::uint32_t) override;
    void FrameRotate(std::uint32_t, std::uint32_t, float,
                     std::uint32_t) override;
    void FrameUpdateObjects(std::uint32_t) override;
    void FrameScale(std::uint32_t, const float*, std::uint32_t) override;
    void FrameTranslate(std::uint32_t, const float*, std::uint32_t) override;
    std::uint32_t ObjectTransform(std::uint32_t) override;
    void ApplyDialogTransform(std::uint32_t, std::uint32_t,
                              std::uint32_t) override;
    void TransformPoints(ResourceCorners&, std::uint32_t) override;
    void CalculateBBox(std::uint32_t*, const ResourceCorners&) override;
    std::uint32_t CalculateCameraPosition(std::uint32_t*, std::uint32_t*,
                                          std::uint32_t) override;
    void ProcessAtomicBounds(std::uint32_t, std::uint32_t,
                             std::uint32_t*) override;
    std::uint32_t ProcessAtomicBounds2(std::uint32_t, std::uint32_t,
                                       std::uint32_t, std::uint32_t*) override;
    std::uint32_t SetClumpMaterialFlagsForAllAtomics(
        std::uint32_t, std::uint32_t) override;
    void PrepareTextureErrorString() override;
    std::uint32_t ReadTexture(std::string_view) override;
    void SetTextureBytes(std::uint32_t, std::uint8_t,
                         std::uint8_t) override;
    void ApplyMaterialPipeline(std::uint32_t,
        std::array<std::uint32_t, 3>&) override;
    void ApplyMaterialPipeline2(std::uint32_t,
        std::array<std::uint32_t, 8>&) override;
    void ApplyMaterialPipeline4(std::uint32_t,
        std::array<std::uint32_t, 3>&) override;
    void ApplyMaterialPipeline5(std::uint32_t,
        std::array<std::uint32_t, 6>&) override;
    void ApplyMaterialPipeline6(std::uint32_t) override;
    std::uint32_t CreateIm3D() override;
    std::uint32_t CheckTexturePathTarget(std::uint32_t,
                                         std::uint32_t) override;
    void DestroyTexture(std::uint32_t) override;
    void ApplyMaterialPipeline3(std::uint32_t, std::uint32_t,
                                std::uint32_t) override;
};

class GameResourceManager final {
public:
    // IDA 0x665b20 and 0x665c30. The 92 words preserve x86 member offsets.
    [[nodiscard]] GameResourceManager& Construct();
    void Destruct(IGameConfigMemory& memory, IRwDictSchemaRuntime& schema_runtime);
    // IDA 0x665cc0 and 0x666080.
    [[nodiscard]] GameResourceManager* LoadClump(
        std::string_view label, std::string_view directory,
        std::string_view model, std::uint32_t texture_context,
        std::uint32_t mode, std::uint32_t extra,
        bool load_effects, IGameResourceManagerRuntime& runtime,
        IRwDictSchemaRuntime& schema_runtime);
    // Final successful-load sequence from IDA0x666007..0x66604c.
    // The caller must have completed resource preparation and own words[5].
    void FinalizeClumpLoad(IGameResourcePoseRuntime& runtime);
    [[nodiscard]] std::uint32_t CleanupClump(
        std::uint32_t texture_context, IGameResourceManagerRuntime& runtime,
        IRwDictSchemaRuntime& schema_runtime);
    // IDA 0x666200 and 0x666350.
    [[nodiscard]] bool LoadShadowClump(std::uint32_t texture_context,
                                       IGameResourceManagerRuntime& runtime);
    // Shared light/frame portion of IDA6662AD..666315 for an owned shadow clump.
    [[nodiscard]] bool InitializeShadowLight(IGameResourceManagerRuntime& runtime);
    [[nodiscard]] bool CleanupShadowClump(std::uint32_t texture_context,
                                          IGameResourceManagerRuntime& runtime);
    // IDA 0x6664a0, 0x666520 and 0x666530.
    [[nodiscard]] std::uint32_t RenderCamera(std::uint32_t enabled,
        std::uint32_t context, IGameResourceManagerRuntime& runtime);
    [[nodiscard]] bool CleanupCamera(IGameResourceManagerRuntime& runtime);
    [[nodiscard]] std::uint32_t SetClumpMaterialFlags(
        std::uint32_t mode, IGameResourceManagerRuntime& runtime);
    // IDA 0x666550, 0x6666e0 and 0x6669d0.
    [[nodiscard]] std::uint32_t UpdateClumpTransform(
        const ResourceVec3& position, bool relative,
        IGameResourcePoseRuntime& runtime);
    void UpdateClumpRotation(const ResourceVec3& rotation, bool relative,
                             IGameResourcePoseRuntime& runtime);
    void UpdateClumpPosition(const ResourceVec3& scale, bool relative,
                             IGameResourcePoseRuntime& runtime);
    // IDA 0x666be0, 0x666dc0, 0x666f60 and 0x666f80.
    [[nodiscard]] std::uint32_t UpdateClumpScale(
        float uniform, bool relative, IGameResourcePoseRuntime& runtime);
    [[nodiscard]] std::uint32_t ResetClumpTransform(
        IGameResourcePoseRuntime& runtime);
    [[nodiscard]] std::uint32_t SetClumpMaterialFlagsForAllAtomics(
        std::uint32_t mode, IGameResourceManagerRuntime& runtime);
    [[nodiscard]] std::uint32_t SetCurrentClump(std::uint32_t clump);
    // IDA 0x666f90 and 0x667060.
    [[nodiscard]] bool LoadTexture(std::string_view name,
                                    std::uint32_t mode,
                                    IGameResourceManagerRuntime& runtime);
    [[nodiscard]] bool SetTexture(std::uint32_t texture,
                                   IGameResourceManagerRuntime& runtime);
    // IDA 0x6670a0.
    [[nodiscard]] bool LoadTextures(std::string_view first,
        std::string_view second, std::uint32_t variant,
        std::uint32_t fourth, std::uint32_t fifth,
        std::uint32_t sixth, std::uint32_t seventh,
        IGameResourceManagerRuntime& runtime);
    // IDA 0x667210..0x667450.
    [[nodiscard]] bool LoadTexture2(std::string_view name,
        std::uint32_t third, std::uint32_t fourth,
        IGameResourceManagerRuntime& runtime);
    [[nodiscard]] bool LoadTexture3(std::uint32_t second,
        std::string_view name, std::uint32_t fourth,
        std::uint32_t fifth, IGameResourceManagerRuntime& runtime);
    [[nodiscard]] bool CleanupTexture(IGameResourceManagerRuntime& runtime);
    [[nodiscard]] bool LoadTexture4(std::string_view name,
        std::uint32_t target, IGameResourceManagerRuntime& runtime);
    // IDA 0x667510, 0x667590 and 0x667620.
    [[nodiscard]] bool RenderClump(std::uint32_t camera,
                                    IGameResourceManagerRuntime& runtime);
    [[nodiscard]] bool RenderClump2(std::uint32_t camera,
                                     IGameResourceManagerRuntime& runtime);
    [[nodiscard]] bool UpdateAndRenderClump(float elapsed,
        IGameResourceFrameRuntime& runtime,
        IRwDictSchemaUpdateRuntime& schema_runtime);
    // IDA 0x6676b0, 0x667990 and 0x667a40.
    void LoadEffectData(IGameResourceManagerRuntime& runtime);
    [[nodiscard]] std::uint32_t LoadExtSizeScl(
        IGameResourceManagerRuntime& runtime);
    [[nodiscard]] std::uint32_t CalculateBoundingBox(
        IGameResourcePoseRuntime& runtime);

    std::array<std::uint32_t, 92> words{};
};

static_assert(sizeof(GameResourceManager) == 92 * sizeof(std::uint32_t));

}  // namespace pal4
