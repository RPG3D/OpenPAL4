#pragma once
#include "pal4/binary_xml_config.h"
#include <array>
#include <istream>
#include <optional>

namespace pal4::portable {
// 原始文件中的数值与拥有所有权的文本；不保留x86对象地址。
struct SceneGob {
    std::uint32_t type{};
    std::string name, resource_directory, model, variant, script;
    std::array<float,3> position{}, rotation{};
    std::array<std::uint32_t,4> flags{};
    BinaryXmlConfig config;
};
struct SceneNpc {
    std::string name, resource, script;
    std::array<float,3> position{}, rotation{};
    std::array<std::uint32_t,3> flags{};
    BinaryXmlConfig behavior, cache;
    std::optional<std::string> DefaultAction() const;
};
struct SceneMonster {
    std::string name,resource;
    std::array<float,3> position{},rotation{};
    std::array<std::uint32_t,5> fields{};
    std::vector<std::uint32_t> values;
    BinaryXmlConfig behavior,cache;
};
struct SceneEvent {
    std::string name, effect, world, scene, script;
    std::uint32_t shape{}, effect_type{};
    std::array<std::uint32_t,2> effect_fields{}, geometry_flags{};
    std::vector<std::array<std::uint32_t,20>> properties;
    std::vector<std::uint8_t> clump;
    // 仅修正已验证的空插件长度；保留原clump供来源核验。
    std::vector<std::uint8_t> normalized_clump;
};
struct SceneCamera {
    std::string name;
    std::array<std::uint32_t,13> values{};
    BinaryXmlConfig config;
};
struct ScenePath {
    std::string name;
    std::uint32_t grid_marker{},radius_bits{};
    std::uint8_t loop_byte{};
    std::vector<std::array<float,3>> points;
};
struct SceneCatalog {
    std::vector<SceneGob> gobs;
    std::vector<SceneNpc> npcs;
    std::vector<SceneMonster> monsters;
    std::vector<ScenePath> paths;
    std::vector<SceneEvent> events;
    std::vector<std::pair<std::string,std::vector<SceneCamera>>> cameras;
};
std::vector<SceneGob> ReadSceneGobs(std::istream& input);
std::vector<SceneNpc> ReadSceneNpcs(std::istream& input);
std::vector<SceneMonster> ReadSceneMonsters(std::istream& input);
std::vector<ScenePath> ReadScenePaths(std::istream& input);
std::vector<SceneEvent> ReadSceneEvents(std::istream& input);
std::vector<SceneCamera> ReadSceneCameras(std::istream& input);
void WriteSceneCatalogReport(std::ostream& output,const SceneCatalog& catalog);
} // namespace pal4::portable
