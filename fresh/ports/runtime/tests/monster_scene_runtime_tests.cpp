#include "pal4/portable/monster_scene_runtime.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace pal4::portable;

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void EmptySceneOwnsAndClosesInDependencyOrder() {
    auto source = std::make_shared<ArenaSource>("runtime-contract-data");
    const std::vector<SceneMonster> monsters;
    const std::vector<ScenePath> paths;
    MonsterSceneRuntime runtime(31, source, monsters, paths, [] { return 16384u; });
    source.reset();

    Check(runtime.Monsters().Size() == 0, "empty scene monster resources");
    Check(runtime.Resources().Paths()->Generation() == 31,
          "runtime preserves the scene generation");
    Check(!runtime.World().Stats().closed, "movement world is live");
    Check(runtime.Movement().UsesWorld(runtime.World()),
          "movement service borrows the owned world");

    const auto borrowed = runtime.World().CreateEndpoint({31, 0}, 7);
    bool refused = false;
    try {
        runtime.Close();
    } catch (const std::logic_error&) {
        refused = true;
    }
    Check(refused, "runtime refuses to close while a helper is borrowed");
    Check(runtime.World().Contains(borrowed), "failed close preserves helper ownership");
    Check(runtime.Movement().UsesWorld(runtime.World()),
          "failed close preserves the movement service");
    runtime.World().DestroyHelper(borrowed);

    runtime.Close();
    Check(runtime.Closed(), "runtime closes all owned services");
    bool rejected = false;
    try {
        (void)runtime.World();
    } catch (const std::logic_error&) {
        rejected = true;
    }
    Check(rejected, "closed runtime rejects world access");
    runtime.Close();
}

void LoadOriginalScene(const std::filesystem::path& game_data,
                       std::string_view world, std::string_view scene) {
    auto source = std::make_shared<ArenaSource>(game_data);
    const std::string prefix = std::string(world) + "\\" + std::string(scene) + "\\";
    const auto monster_bytes = source->ReadSceneMember(prefix + "mstInfo.mst");
    const auto path_bytes = source->ReadSceneMember(prefix + "pathway.pwy");
    Check(monster_bytes.has_value(), "original scene has mstInfo.mst");
    Check(path_bytes.has_value(), "original scene has pathway.pwy");

    std::istringstream monster_input(std::string(monster_bytes->begin(), monster_bytes->end()));
    std::istringstream path_input(std::string(path_bytes->begin(), path_bytes->end()));
    const auto monsters = ReadSceneMonsters(monster_input);
    const auto paths = ReadScenePaths(path_input);
    Check(!monsters.empty(), "original scene is populated with monsters");

    MonsterSceneRuntime runtime(41, source, monsters, paths, [] { return 16384u; });
    source.reset();
    Check(runtime.Monsters().Size() == monsters.size(), "all original monsters are loaded");
    Check(runtime.Monsters().ModelCount() != 0, "original monster models are loaded");
    Check(runtime.Resources().Paths()->Size() == paths.size(), "all original pathways are retained");
    Check(runtime.World().Stats().helpers == 0, "world starts with no fabricated helpers");
    Check(runtime.Movement().UsesWorld(runtime.World()), "movement uses the original scene world");
    runtime.Close();
    Check(runtime.Closed(), "populated runtime closes");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            EmptySceneOwnsAndClosesInDependencyOrder();
        } else if (argc == 4) {
            LoadOriginalScene(argv[1], argv[2], argv[3]);
        } else {
            throw std::invalid_argument("expected no arguments or <game-data> <world> <scene>");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
