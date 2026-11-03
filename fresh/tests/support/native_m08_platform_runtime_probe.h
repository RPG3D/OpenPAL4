#pragma once
#include <filesystem>
namespace pal4 {
class IGameResourceManagerRuntime;
class NativeBaseObjectMemory;
class IRwDictSchemaRuntime;
namespace test {
void VerifyNativeM08PlatformRuntime(const std::filesystem::path&,
    IGameResourceManagerRuntime&, NativeBaseObjectMemory&, IRwDictSchemaRuntime&);
}
}
