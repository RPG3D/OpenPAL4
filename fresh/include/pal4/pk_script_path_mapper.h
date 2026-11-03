#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

class PkScriptPathMapper final {
public:
    [[nodiscard]] static std::uint32_t FindString(
        std::string_view source, std::string_view pattern,
        std::uint32_t start);
    static void PalActor(const char* source, char* destination,
                         std::size_t count);
    static void PalObject(const char* source, char* destination,
                          std::size_t count);
    static void PalWorld(const char* source, char* destination,
                         std::size_t count);
    static void CombatWorld(const char* source, char* destination,
                            std::size_t count);
    static void SceneData(const char* source, char* destination,
                          std::size_t count);
    static void Database(const char* source, char* destination,
                         std::size_t count);
    static void Script(const char* source, char* destination,
                       std::size_t count);
    static void Music(const char* source, char* destination,
                      std::size_t count);
    static void Video(const char* source, char* destination,
                      std::size_t count);
    static void Weapon(const char* source, char* destination,
                       std::size_t count);
    static void Ui(const char* source, char* destination,
                   std::size_t count);
    static void Effect(const char* source, char* destination,
                       std::size_t count);
    static void MatFx(const char* source, char* destination,
                      std::size_t count);
    static void TwoD(const char* source, char* destination,
                     std::size_t count);

private:
    static void Rewrite(const char* source, const char* prefix,
                        char* destination, std::size_t count,
                        bool discard_first_directory = false);
};

}  // namespace pal4
