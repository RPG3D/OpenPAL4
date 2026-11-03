#pragma once

#include "pal4/archive_text_reader.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

// Provides the original database member as an in-memory archive stream.
class NativeGameDatabaseSource final : public IArchiveStreamRuntime,
                                       private IArchiveTextReadRuntime {
public:
    static constexpr std::uint32_t kStreamHandle = 1u;
    [[nodiscard]] bool Open(const std::filesystem::path& game_root,
                            std::string_view member = "pal4db.db");
    void Rewind() { position_ = 0u; }
    [[nodiscard]] ArchiveStreamAdapterState StreamState() const {
        return {kStreamHandle, 1u};
    }
    [[nodiscard]] std::optional<std::uint32_t> ReadWord();
    [[nodiscard]] std::optional<std::string> ReadText();
    [[nodiscard]] std::size_t Position() const { return position_; }
    [[nodiscard]] std::size_t Size() const { return bytes_.size(); }

    [[nodiscard]] std::uint32_t InvokeVirtual(
        std::uint32_t stream, std::uint32_t slot_offset,
        std::uintptr_t buffer, std::uint32_t bytes) override;

private:
    [[nodiscard]] const char* EmptyData() override { return ""; }
    [[nodiscard]] bool AllocateText(StringBufferState& text,
                                    std::uint32_t length,
                                    std::uint8_t mode) override;
    void SetErrorFlag(StringBufferState& text, std::uint8_t flag) override;
    void Reallocate(StringBufferState& text, std::uint32_t length) override;
    void SetTextSize(StringBufferState& text, std::uint32_t length) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Copy(std::uint32_t destination, const char* source,
              std::uint32_t length) override;

    std::vector<std::uint8_t> bytes_;
    std::vector<char> text_storage_;
    std::size_t position_ = 0u;
};

}  // namespace pal4
