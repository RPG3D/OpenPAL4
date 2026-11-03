#pragma once
#include <filesystem>
struct RpWorld;struct RwCamera;
namespace pal4 {
class NativePkFileManagerRuntime;class NativePalGameIVTextureStorage;class NativeGameDatabase;
namespace test {
void VerifyNativeSmithPreview(const std::filesystem::path&,NativePkFileManagerRuntime&,NativePalGameIVTextureStorage&,
    NativeGameDatabase&,RpWorld*,RwCamera*);
void VerifyNativeSmithOwner(const std::filesystem::path&,NativePkFileManagerRuntime&,NativePalGameIVTextureStorage&,
    NativeGameDatabase&,RpWorld*,RwCamera*);
}
}
