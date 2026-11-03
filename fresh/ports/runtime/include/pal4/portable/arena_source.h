#pragma once
#include "pal4/portable/arena.h"
#include "pal4/portable/original_database.h"
#include <filesystem>
#include <iosfwd>
#include <memory>
namespace pal4::portable {
class OriginalAnimationStore;
struct OriginalAnimationCatalog {
    std::vector<std::string> names;
    std::shared_ptr<OriginalAnimationStore> store;
};
// 单参数构造只允许原始 CPK；多参数构造供旧开发探针读取转换数据。
class ArenaSource {
public:
    explicit ArenaSource(std::filesystem::path game_data);
    ArenaSource(std::filesystem::path game_data,std::filesystem::path cache,ArenaAssetReader development_assets={});
    ~ArenaSource();
    ArenaSource(const ArenaSource&)=delete;
    ArenaSource& operator=(const ArenaSource&)=delete;
    CollisionMesh ReadOriginalCollision(std::string_view world,std::string_view scene);
    ArenaDescription ReadOriginalDescription(std::string_view world,std::string_view scene,std::span<const OriginalSceneDefinition> scenes);
    Scene ReadOriginalEnvironment(std::string_view world,std::string_view scene,std::string_view kind);
    Scene ReadOriginalWorld(std::string_view world,std::string_view scene);
    Actor ReadOriginalActorResource(std::string_view directory,std::string_view model,std::string_view variant={},bool lazy_animations=false);
    OriginalAnimationCatalog OpenOriginalAnimations(std::string_view directory,std::string_view model);
    std::vector<std::uint8_t> Read(std::string_view member);
    std::optional<std::vector<std::uint8_t>> ReadSceneMember(std::string_view member);
    std::optional<std::vector<std::uint8_t>> ReadObjectMember(std::string_view directory,std::string_view member);
    // 释放场景包及未被延迟动画持有的资源包；后续读取时按需重新打开。
    std::size_t ReleaseUnusedArchives();
    void WriteReport(std::ostream& output) const;
private:
    OriginalAnimationCatalog CreateOriginalAnimations(std::string_view directory,std::vector<std::uint32_t> hierarchy);
    Texture ReadOriginalTexture(std::string_view directory,std::string_view name);
    struct Impl;std::shared_ptr<Impl> impl_;
};
}
