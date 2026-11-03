#pragma once
#include <string_view>
namespace pal4 {
struct GameWorldObjectState;
class NativeWorldAsset;
class NativePackageArchive;
void PrepareNativeGameWorldMaterials(GameWorldObjectState& state,NativeWorldAsset& asset,
    NativePackageArchive& archive,std::string_view directory);
}
