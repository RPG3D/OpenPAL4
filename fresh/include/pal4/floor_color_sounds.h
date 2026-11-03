#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace pal4 {
struct FloorColorSound { std::uint32_t rgb; std::string_view path; };
inline constexpr std::array<FloorColorSound, 15> kColorSounds{{
    {0x0099ccffu, "gamedata\\palSound\\WF011.wav"},
    {0x00003399u, "gamedata\\palSound\\WF013.wav"},
    {0x000099ffu, "gamedata\\palSound\\WF015.wav"},
    {0x00969696u, "gamedata\\palSound\\WF017.wav"},
    {0x00cccc33u, "gamedata\\palSound\\WF019.wav"},
    {0x00ffffffu, "gamedata\\palSound\\WF021.wav"},
    {0x0099ffffu, "gamedata\\palSound\\WF023.wav"},
    {0x00008080u, "gamedata\\palSound\\WF037.wav"},
    {0x00ffcc99u, "gamedata\\palSound\\WF039.wav"},
    {0x00ccffccu, "gamedata\\palSound\\WF041.wav"},
    {0x00cc99ffu, "gamedata\\palSound\\WF053.wav"},
    {0x00ff99ccu, "gamedata\\palSound\\WF021.wav"},
    {0x00ff6633u, "gamedata\\palSound\\WF039.wav"},
    {0x00ff00ffu, "gamedata\\palSound\\WF021.wav"},
    {0x000000ffu, "gamedata\\palSound\\WF017.wav"},
}};
} // namespace pal4
