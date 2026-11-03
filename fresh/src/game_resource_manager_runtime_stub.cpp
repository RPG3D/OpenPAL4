#include "pal4/game_resource_manager.h"

namespace pal4 {

std::uint32_t GameResourceManagerRuntimeStub::Free(std::uint32_t) { return 0; }
std::uint32_t GameResourceManagerRuntimeStub::Allocate(std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::StoreString(std::uint32_t, std::string_view) {}
std::string GameResourceManagerRuntimeStub::ReadString(std::uint32_t) { return {}; }
void GameResourceManagerRuntimeStub::CleanupCamera() {}
std::uint32_t GameResourceManagerRuntimeStub::CreateLight(std::uint32_t) { return 0; }
std::uint32_t GameResourceManagerRuntimeStub::CreateFrame() { return 0; }
std::uint32_t GameResourceManagerRuntimeStub::LightFrame(std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::SetLightFrame(std::uint32_t, std::uint32_t) {}
void GameResourceManagerRuntimeStub::AddFrameChild(std::uint32_t, std::uint32_t) {}
void GameResourceManagerRuntimeStub::DestroyFrame(std::uint32_t) {}
void GameResourceManagerRuntimeStub::DestroyLight(std::uint32_t) {}
void GameResourceManagerRuntimeStub::DestroyIm3D(std::uint32_t) {}
std::uint32_t GameResourceManagerRuntimeStub::ClumpWorld(std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::RemoveClump(std::uint32_t, std::uint32_t) {}
void GameResourceManagerRuntimeStub::DestroyClump(std::uint32_t) {}
void GameResourceManagerRuntimeStub::RemoveTextureByName(std::uint32_t,
                                                         std::string_view) {}
void GameResourceManagerRuntimeStub::ImageReallocateAndCopy(std::uint32_t) {}
std::uint32_t GameResourceManagerRuntimeStub::LoadWorldClump(
    std::string_view, std::uint32_t, std::uint32_t) { return 0; }
std::uint32_t GameResourceManagerRuntimeStub::SetClumpMaterialFlagsRaw(
    std::uint32_t, std::uint32_t) { return 0; }
std::uint32_t GameResourceManagerRuntimeStub::InitCamera(
    std::uint32_t*, std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t*, std::uint32_t*) { return 0; }
void GameResourceManagerRuntimeStub::RenderCameraScene(
    std::uint32_t*, float, std::uint32_t*) {}
bool GameResourceManagerRuntimeStub::FrustumTestSphere(
    std::uint32_t, std::uint32_t*) { return false; }
void GameResourceManagerRuntimeStub::RenderClumpRaw(std::uint32_t) {}
void GameResourceManagerRuntimeStub::RenderCameraRaw(
    std::uint32_t*, std::uint32_t) {}
std::uint32_t GameResourceManagerRuntimeStub::ReadGeometryWord(
    std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::WriteGeometryWord(
    std::uint32_t, std::uint32_t) {}
std::uint32_t GameResourceManagerRuntimeStub::ForAllGeometryFlagAtomics(
    std::uint32_t,
    std::uint32_t (*)(std::uint32_t, std::uint32_t,
                       IRwGeometryFlagsRuntime&),
    std::uint32_t) { return 0; }
std::uint32_t GameResourceManagerRuntimeStub::ReadOptimizerWord(
    std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::WriteOptimizerWord(
    std::uint32_t, std::uint32_t) {}
void GameResourceManagerRuntimeStub::SetTriStripMethod(std::uint32_t) {}
std::uint8_t GameResourceManagerRuntimeStub::TriStripMeshFlag() { return 0; }
std::uint32_t GameResourceManagerRuntimeStub::ForAllOptimizerMeshes(
    std::uint32_t,
    std::uint32_t (*)(std::uint32_t, std::uint32_t,
                       std::uint32_t, IRwClumpOptimizerRuntime&),
    std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::OptimizeAtomicRaw(std::uint32_t) {}
void GameResourceManagerRuntimeStub::OptimizeClumpRaw(
    std::uint32_t, std::uint32_t) {}
std::uint32_t GameResourceManagerRuntimeStub::ForAllOptimizerAtomics(
    std::uint32_t,
    std::uint32_t (*)(std::uint32_t, std::uint32_t,
                       IRwClumpOptimizerRuntime&),
    std::uint32_t) { return 0; }
bool GameResourceManagerRuntimeStub::CheckTexturePath(std::uint32_t) { return false; }
std::uint32_t GameResourceManagerRuntimeStub::FirstMaterialAtomic(std::uint32_t) { return 0; }
std::string GameResourceManagerRuntimeStub::TexturePath(std::uint32_t) { return {}; }
std::string GameResourceManagerRuntimeStub::SplitFilename(std::string_view) { return {}; }
std::uint32_t GameResourceManagerRuntimeStub::OpenDataStream(
    std::uint32_t, std::uint32_t, std::string_view) { return 0; }
void GameResourceManagerRuntimeStub::ReadStreamWord(
    std::uint32_t, std::uint32_t*) {}
void GameResourceManagerRuntimeStub::ReadStreamWords(
    std::uint32_t, std::uint32_t*, std::uint32_t) {}
void GameResourceManagerRuntimeStub::ReadStreamBuffer(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void GameResourceManagerRuntimeStub::TerminateBuffer(
    std::uint32_t, std::uint32_t) {}
std::uint32_t GameResourceManagerRuntimeStub::CloseDataStream(
    std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::AddAndProcessScript8() {}
void GameResourceManagerRuntimeStub::CleanupAllScripts() {}
std::uint32_t GameResourceManagerRuntimeStub::ClumpFrame(std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::FrameSetIdentity(std::uint32_t) {}
void GameResourceManagerRuntimeStub::FrameRotate(std::uint32_t, std::uint32_t,
                                                  float, std::uint32_t) {}
void GameResourceManagerRuntimeStub::FrameUpdateObjects(std::uint32_t) {}
void GameResourceManagerRuntimeStub::FrameScale(std::uint32_t, const float*,
                                                std::uint32_t) {}
void GameResourceManagerRuntimeStub::FrameTranslate(std::uint32_t,
                                                     const float*,
                                                     std::uint32_t) {}
std::uint32_t GameResourceManagerRuntimeStub::ObjectTransform(std::uint32_t) {
    return 0;
}
void GameResourceManagerRuntimeStub::ApplyDialogTransform(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void GameResourceManagerRuntimeStub::TransformPoints(
    ResourceCorners&, std::uint32_t) {}
void GameResourceManagerRuntimeStub::CalculateBBox(
    std::uint32_t*, const ResourceCorners&) {}
std::uint32_t GameResourceManagerRuntimeStub::CalculateCameraPosition(
    std::uint32_t*, std::uint32_t*, std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::ProcessAtomicBounds(
    std::uint32_t, std::uint32_t, std::uint32_t*) {}
std::uint32_t GameResourceManagerRuntimeStub::ProcessAtomicBounds2(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t*) { return 0; }
std::uint32_t GameResourceManagerRuntimeStub::SetClumpMaterialFlagsForAllAtomics(
    std::uint32_t, std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::PrepareTextureErrorString() {}
std::uint32_t GameResourceManagerRuntimeStub::ReadTexture(std::string_view) {
    return 0;
}
void GameResourceManagerRuntimeStub::SetTextureBytes(
    std::uint32_t, std::uint8_t, std::uint8_t) {}
void GameResourceManagerRuntimeStub::ApplyMaterialPipeline(
    std::uint32_t, std::array<std::uint32_t, 3>&) {}
void GameResourceManagerRuntimeStub::ApplyMaterialPipeline2(
    std::uint32_t, std::array<std::uint32_t, 8>&) {}
void GameResourceManagerRuntimeStub::ApplyMaterialPipeline4(
    std::uint32_t, std::array<std::uint32_t, 3>&) {}
void GameResourceManagerRuntimeStub::ApplyMaterialPipeline5(
    std::uint32_t, std::array<std::uint32_t, 6>&) {}
void GameResourceManagerRuntimeStub::ApplyMaterialPipeline6(std::uint32_t) {}
std::uint32_t GameResourceManagerRuntimeStub::CreateIm3D() { return 0; }
std::uint32_t GameResourceManagerRuntimeStub::CheckTexturePathTarget(
    std::uint32_t, std::uint32_t) { return 0; }
void GameResourceManagerRuntimeStub::DestroyTexture(std::uint32_t) {}
void GameResourceManagerRuntimeStub::ApplyMaterialPipeline3(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

}  // namespace pal4
