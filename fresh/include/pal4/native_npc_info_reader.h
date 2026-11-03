#pragma once

#include "pal4/binary_xml_config.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace pal4 {

struct NativeNpcInfoRecord {
    std::string name;
    std::string resource;
    std::string reserved;
    std::array<std::uint32_t, 3> position_bits{};
    std::array<std::uint32_t, 3> rotation_bits{};
    std::array<std::uint32_t, 3> flags{};
    BinaryXmlConfig behavior;
    BinaryXmlConfig buffer_cache;

    [[nodiscard]] std::optional<std::string> DefaultAction() const;
};

class NativeNpcInfoReader final {
public:
    enum class LoadStatus {
        NotLoaded, Ready, MissingMember, InvalidRequest, ArchiveError, InvalidData
    };
    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            const std::string& world,
                            const std::string& scene);
    [[nodiscard]] bool Parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] const std::vector<NativeNpcInfoRecord>& Records() const {
        return records_;
    }
    [[nodiscard]] const std::string& LastError() const { return last_error_; }
    [[nodiscard]] LoadStatus Status() const noexcept { return status_; }

private:
    std::vector<NativeNpcInfoRecord> records_;
    std::string last_error_;
    LoadStatus status_=LoadStatus::NotLoaded;
};

}  // namespace pal4
