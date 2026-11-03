#pragma once
#include <filesystem>
struct RpWorld;
struct RwCamera;
namespace pal4 {
class NativePkFileManagerRuntime;
class NativePalGameIVTextureStorage;
class NativeGameDatabase;
class NativeCeguiUiSession;
class NativeArenaLoadRuntime;
namespace test {
void VerifyNativeTradeWindowComposition(const std::filesystem::path&,NativeCeguiUiSession&,const NativeGameDatabase&,NativePkFileManagerRuntime&,NativePalGameIVTextureStorage&,RpWorld*,RwCamera*);
void VerifyNativeTradeWindowDetails(const std::filesystem::path&,NativeCeguiUiSession&,const NativeGameDatabase&,NativePkFileManagerRuntime&,NativePalGameIVTextureStorage&,RpWorld*,RwCamera*);
void VerifyNativeTradeWindowConfirmation(const std::filesystem::path&,NativeCeguiUiSession&,const NativeGameDatabase&);
void VerifyNativeTradeWindowInput(const std::filesystem::path&,NativeCeguiUiSession&,RwCamera*);
void VerifyNativeTradeWindowData(const std::filesystem::path&,NativeCeguiUiSession&,const NativeGameDatabase&,NativePalGameIVTextureStorage&);
void VerifyNativeTradeWindowLayout(const std::filesystem::path&,NativeCeguiUiSession&);
void VerifyNativeShopSelectorWindow(const std::filesystem::path&,NativeCeguiUiSession&,
    const NativeGameDatabase&,NativePalGameIVTextureStorage&,NativeArenaLoadRuntime&,NativePkFileManagerRuntime&);
void VerifyNativeShopSelectorLayout(const std::filesystem::path& root,
    NativeCeguiUiSession&,const NativeGameDatabase&,NativePalGameIVTextureStorage&,RwCamera*);
void VerifyNativeSystemPropertyFactory(const std::filesystem::path& root,
    NativePkFileManagerRuntime& packages, NativePalGameIVTextureStorage& game,
    NativeGameDatabase& database, RpWorld* world, RwCamera* camera);
}
}
