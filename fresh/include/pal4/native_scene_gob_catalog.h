#pragma once

#include "pal4/binary_xml_config.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

struct NativeSceneGobRecord final {
    std::uint32_t type = 0u;
    std::string name;
    std::string resource_dir;
    std::string model;
    std::string variant;
    std::array<std::uint32_t, 3> position_bits{};
    std::array<std::uint32_t, 3> rotation_bits{};
    std::string script_entry;
    std::array<std::uint32_t, 4> flags{};
    BinaryXmlConfig config;

    [[nodiscard]] std::optional<std::string_view> EffectName() const;
};

// Reads the scene's original GameObjs.gob data without constructing GOB actors.
class NativeSceneGobCatalog final {
public:
    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view region,
                            std::string_view scene);
    [[nodiscard]] const std::vector<NativeSceneGobRecord>& Records() const
        noexcept { return records_; }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }
    [[nodiscard]] bool SceneMemberMissing() const noexcept { return scene_member_missing_; }

private:
    std::vector<NativeSceneGobRecord> records_;
    std::string last_error_;
    bool scene_member_missing_{};
};

}  // namespace pal4
