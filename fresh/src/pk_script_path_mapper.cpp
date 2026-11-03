#include "pal4/pk_script_path_mapper.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace pal4 {
namespace {

void LowercaseAscii(std::string& value) {
    for (auto& character : value)
        if (character >= 'A' && character <= 'Z')
            character = static_cast<char>(character - 'A' + 'a');
}

}  // namespace

std::uint32_t PkScriptPathMapper::FindString(
    const std::string_view source, const std::string_view pattern,
    const std::uint32_t start) {
    const auto found = source.find(pattern, start);
    return found == std::string_view::npos
               ? 0xffffffffu : static_cast<std::uint32_t>(found);
}

void PkScriptPathMapper::Rewrite(
    const char* source, const char* prefix, char* destination,
    const std::size_t count, const bool discard_first_directory) {
    if (destination == nullptr || count == 0) return;
    std::string path(source != nullptr ? source : "");
    std::string marker(prefix);
    LowercaseAscii(path);
    LowercaseAscii(marker);
    if (path.rfind(marker) != std::string::npos) {
        path.erase(0, marker.size());
        if (discard_first_directory) {
            const auto separator = FindString(path, "\\", 0u);
            if (separator != 0xffffffffu)
                path.erase(0, separator + 1u);
        }
    }
    const auto copied = std::min(count, path.size());
    std::memcpy(destination, path.data(), copied);
    if (copied < count)
        std::memset(destination + copied, 0, count - copied);
}

void PkScriptPathMapper::PalActor(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\PALActor\\", destination, count);
}

void PkScriptPathMapper::PalObject(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\PALObject\\", destination, count);
}

void PkScriptPathMapper::PalWorld(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\PALWorld\\", destination, count, true);
}

void PkScriptPathMapper::CombatWorld(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\PALWorld\\CombatWorld\\",
            destination, count);
}

void PkScriptPathMapper::SceneData(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\editData\\scene\\", destination, count);
}

void PkScriptPathMapper::Database(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\database\\", destination, count);
}

void PkScriptPathMapper::Script(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\EditData\\Script\\", destination, count);
}

void PkScriptPathMapper::Music(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\Music\\", destination, count);
}

void PkScriptPathMapper::Video(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\Video\\", destination, count);
}

void PkScriptPathMapper::Weapon(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\PALWeapon\\", destination, count);
}

void PkScriptPathMapper::Ui(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\UI\\", destination, count);
}

void PkScriptPathMapper::Effect(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\Effect\\", destination, count);
}

void PkScriptPathMapper::MatFx(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\MatFX\\", destination, count);
}

void PkScriptPathMapper::TwoD(
    const char* source, char* destination, const std::size_t count) {
    Rewrite(source, "gamedata\\2D\\", destination, count);
}

}  // namespace pal4
