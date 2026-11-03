#include "pal4/pk_script_path_mapper.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>

namespace {

using Mapper = void (*)(const char*, char*, std::size_t);

int failures = 0;
void Check(bool okay, const char* message) {
    if (!okay) {
        ++failures;
        std::cerr << message << '\n';
    }
}

void TestEveryFixedPrefix() {
    struct Case {
        Mapper map;
        const char* prefix;
        const char* suffix;
        const char* expected;
    };
    const std::array<Case, 14> cases{{
        {pal4::PkScriptPathMapper::PalActor,
         "gamedata\\PALActor\\", "Hero.BIN", "hero.bin"},
        {pal4::PkScriptPathMapper::PalObject,
         "gamedata\\PALObject\\", "Prop.BIN", "prop.bin"},
        {pal4::PkScriptPathMapper::PalWorld,
         "gamedata\\PALWorld\\", "AREA\\Map.BIN", "map.bin"},
        {pal4::PkScriptPathMapper::CombatWorld,
         "gamedata\\PALWorld\\CombatWorld\\", "Arena.BIN", "arena.bin"},
        {pal4::PkScriptPathMapper::SceneData,
         "gamedata\\editData\\scene\\", "Scene.BIN", "scene.bin"},
        {pal4::PkScriptPathMapper::Database,
         "gamedata\\database\\", "Table.BIN", "table.bin"},
        {pal4::PkScriptPathMapper::Script,
         "gamedata\\EditData\\Script\\", "Event.BIN", "event.bin"},
        {pal4::PkScriptPathMapper::Music,
         "gamedata\\Music\\", "Theme.BIN", "theme.bin"},
        {pal4::PkScriptPathMapper::Video,
         "gamedata\\Video\\", "Intro.BIN", "intro.bin"},
        {pal4::PkScriptPathMapper::Weapon,
         "gamedata\\PALWeapon\\", "Sword.BIN", "sword.bin"},
        {pal4::PkScriptPathMapper::Ui,
         "gamedata\\UI\\", "Panel.BIN", "panel.bin"},
        {pal4::PkScriptPathMapper::Effect,
         "gamedata\\Effect\\", "Glow.BIN", "glow.bin"},
        {pal4::PkScriptPathMapper::MatFx,
         "gamedata\\MatFX\\", "Metal.BIN", "metal.bin"},
        {pal4::PkScriptPathMapper::TwoD,
         "gamedata\\2D\\", "Icon.BIN", "icon.bin"},
    }};
    for (const auto& entry : cases) {
        std::array<char, 128> destination{};
        const std::string input = std::string(entry.prefix) + entry.suffix;
        entry.map(input.c_str(), destination.data(), destination.size());
        Check(std::strcmp(destination.data(), entry.expected) == 0,
              "fixed prefix lowercases and strips to expected relative path");
    }
}

void TestSearchAndCopySemantics() {
    std::array<char, 64> destination{};
    pal4::PkScriptPathMapper::PalActor(
        "Other\\HERO.BIN", destination.data(), destination.size());
    Check(std::strcmp(destination.data(), "other\\hero.bin") == 0,
          "missing marker keeps lowercased path");

    destination.fill('?');
    pal4::PkScriptPathMapper::PalActor(
        "Xgamedata\\PALActor\\Hero.BIN",
        destination.data(), destination.size());
    Check(std::strcmp(destination.data(), "\\hero.bin") == 0,
          "native reverse search tests containment then strips fixed length");

    destination.fill('?');
    pal4::PkScriptPathMapper::PalWorld(
        "gamedata\\PALWorld\\Map.BIN",
        destination.data(), destination.size());
    Check(std::strcmp(destination.data(), "map.bin") == 0,
          "world path without a child directory keeps its basename");

    destination.fill('?');
    pal4::PkScriptPathMapper::PalActor(
        "gamedata\\PALActor\\Hero.BIN", destination.data(), 4u);
    Check(std::memcmp(destination.data(), "hero", 4u) == 0 &&
              destination[4] == '?',
          "count-limited CRT copy does not append a terminator when truncated");

    destination.fill('?');
    pal4::PkScriptPathMapper::PalActor(
        "gamedata\\PALActor\\Hero.BIN", destination.data(), 0u);
    Check(destination[0] == '?', "zero count leaves output untouched");
}

void TestFindString() {
    using pal4::PkScriptPathMapper;
    Check(PkScriptPathMapper::FindString("aa\\bb\\cc", "\\", 0u) == 2u &&
              PkScriptPathMapper::FindString("aa\\bb\\cc", "\\", 3u) == 5u,
          "forward find starts at requested offset");
    Check(PkScriptPathMapper::FindString("abcdef", "cd", 0u) == 2u &&
              PkScriptPathMapper::FindString("abcdef", "cd", 3u) ==
                  0xffffffffu,
          "forward find matches full explicit pattern or returns minus one");
    Check(PkScriptPathMapper::FindString("abc", "", 3u) == 3u &&
              PkScriptPathMapper::FindString("abc", "", 4u) ==
                  0xffffffffu &&
              PkScriptPathMapper::FindString("abc", "abcd", 0u) ==
                  0xffffffffu,
          "empty, out-of-range and longer-pattern boundaries");
    const std::string source{"a\0bc", 4};
    const std::string pattern{"\0b", 2};
    Check(PkScriptPathMapper::FindString(source, pattern, 0u) == 1u,
          "explicit lengths allow embedded zero bytes");
}

}  // namespace

int main() {
    TestEveryFixedPrefix();
    TestSearchAndCopySemantics();
    TestFindString();
    return failures == 0 ? 0 : 1;
}
