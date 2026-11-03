#pragma once

#include "pal4/binary_xml_config.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace pal4 {

struct NativeMonsterInfoRecord {
    std::string name;
    std::string resource;
    std::array<std::uint32_t, 3> position_bits{};
    std::array<std::uint32_t, 3> rotation_bits{};
    std::array<std::uint32_t, 5> fields{};
    std::vector<std::uint32_t> values;
    BinaryXmlConfig behavior;
    BinaryXmlConfig buffer_cache;
};

class NativeMonsterInfoReader final {
public:
    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            const std::string& world,
                            const std::string& scene);
    [[nodiscard]] bool Parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] const std::vector<NativeMonsterInfoRecord>& Records() const {
        return records_;
    }
    [[nodiscard]] const std::string& LastError() const { return last_error_; }
    [[nodiscard]] bool SceneMemberMissing() const noexcept {
        return scene_member_missing_;
    }

private:
    std::vector<NativeMonsterInfoRecord> records_;
    std::string last_error_;
    bool scene_member_missing_ = false;
};

}  // namespace pal4
