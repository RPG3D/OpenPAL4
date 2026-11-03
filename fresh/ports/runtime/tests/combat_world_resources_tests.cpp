#include "pal4/portable/combat_world_resources.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using pal4::portable::CombatWorldResources;

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

template<class F> std::string Throws(F&& action) {
    try { action(); }
    catch (const std::exception& error) { return error.what(); }
    throw std::runtime_error("expected CombatWorld resource rejection");
}

void InvalidNamesAndMissingPackage() {
    CombatWorldResources resources(
        std::filesystem::temp_directory_path() / "pal4-combat-world-no-game-data");
    for (const auto bad : {"", "../M01", "M01/../M02", "M01.cpk", "a\\b"}) {
        bool rejected{};
        try { resources.Load(bad); }
        catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected && !resources.Loaded(), "unsafe area name is rejected before IO");
    }
    const auto missing = Throws([&] { resources.Load("M01"); });
    Check(missing.find("M01.cpk") != std::string::npos && !resources.Loaded(),
          "missing original package leaves the owner empty");
    bool empty_borrow_rejected{};
    try { (void)resources.Borrow(); }
    catch (const std::logic_error&) { empty_borrow_rejected = true; }
    Check(empty_borrow_rejected, "an unloaded owner cannot lend a world");
}

void LoadOriginalCombatWorld(const std::filesystem::path& game_data) {
    CombatWorldResources resources(game_data);
    resources.Load("M01");
    Check(resources.Loaded(), "original M01 CombatWorld is loaded");
    {
        auto world = resources.Borrow();
        Check(world.Area() == "M01" && world.World().vertices.size() >= 3u &&
              world.World().vertices.size() % 3u == 0u &&
              !world.World().draws.empty() && !world.World().textures.empty(),
              "original BSP becomes a textured portable Scene");
        Check(world.LightmapConfig()[0] != 0u ||
              world.LightmapConfig()[12] != 0u,
              "original 16-byte lightmap config is retained");

        resources.Load("M01");
        bool replacement_rejected{};
        try { resources.Load("M02"); }
        catch (const std::logic_error&) { replacement_rejected = true; }
        bool reset_rejected{};
        try { resources.Reset(); }
        catch (const std::logic_error&) { reset_rejected = true; }
        Check(replacement_rejected && reset_rejected && resources.Loaded(),
              "same-area load is stable and lease blocks replacement/reset");
    }

    resources.Reset();
    Check(!resources.Loaded(), "reset releases scene and texture data");
    const auto missing_area = Throws([&] { resources.Load("M02"); });
    Check(missing_area.find("M02.cpk") != std::string::npos && !resources.Loaded(),
          "area is used to resolve the archive rather than fixed to M01");
    resources.Load("M01");
    Check(resources.Loaded(), "owner is reusable after failed replacement attempt");
    resources.Reset();
}
} // namespace

int main(int argc, char** argv) {
    try {
        InvalidNamesAndMissingPackage();
        if (argc > 1) LoadOriginalCombatWorld(argv[1]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
