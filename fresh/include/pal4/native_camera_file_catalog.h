#pragma once

#include "pal4/binary_xml_config.h"

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

struct NativeCameraFileRecord {
    std::string name;
    std::array<std::uint32_t, 17u> image{};
    std::unique_ptr<char[]> name_data;
    BinaryXmlConfig xml;

    [[nodiscard]] std::uint32_t Address() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(image.data()));
    }
};

// CameraRecord::Read (IDA 0x524e50): name, 52 data bytes, then XML.
class NativeCameraFileCatalog final {
public:
    // ScriptCloudManager_Clear (0x524f90), using the existing RAII records.
    void Clear() noexcept { records_.clear(); }
    [[nodiscard]] bool Parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] std::size_t Count() const noexcept { return records_.size(); }
    [[nodiscard]] const NativeCameraFileRecord* Find(
        std::string_view name) const noexcept;
    [[nodiscard]] const NativeCameraFileRecord& At(std::size_t index) const {
        return records_.at(index);
    }

private:
    std::vector<NativeCameraFileRecord> records_;
};

}  // namespace pal4
