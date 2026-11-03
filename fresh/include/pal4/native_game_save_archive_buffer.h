#pragma once

#include "pal4/archive_stream_adapter.h"
#include "pal4/archive_text_writer.h"
#include "pal4/binary_xml_config.h"
#include "pal4/game_save_writer.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace pal4 {

class NativeGameSaveArchiveBuffer final : private IArchiveStreamRuntime,
                                          private IArchiveTextRuntime {
public:
    [[nodiscard]] bool WritePrefix(std::string_view marker,
                                   const GameSaveHeader& header);
    [[nodiscard]] bool WriteMarker(std::string_view marker);
    [[nodiscard]] bool WriteConfig(const BinaryXmlNode& root);
    [[nodiscard]] bool WriteBytes(std::span<const std::byte> bytes);
    [[nodiscard]] const std::vector<std::byte>& Bytes() const noexcept {
        return bytes_;
    }

private:
    [[nodiscard]] std::uint32_t InvokeVirtual(
        std::uint32_t stream, std::uint32_t slot_offset,
        std::uintptr_t buffer, std::uint32_t bytes) override;
    [[nodiscard]] const void* DataAt(std::uint32_t address) override;
    [[nodiscard]] const void* EmptyData() override;

    std::vector<std::byte> bytes_;
};

}  // namespace pal4
