#pragma once

#include "pal4/archive_text_reader.h"
#include "pal4/binary_xml_config.h"
#include <span>
#include "pal4/game_save_writer.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace pal4 {

// Owns one open native save stream. Reads retain the exact archive cursor for
// subsequent subsystem deserializers. Modern STL owns text/XML storage.
class NativeGameSaveArchiveReader final : private IArchiveStreamRuntime,
                                         private IArchiveTextReadRuntime {
public:
    ~NativeGameSaveArchiveReader() override { Close(); }
    [[nodiscard]] bool OpenSlot(const std::filesystem::path& game_root, std::int32_t slot);
    void Close() noexcept;
    [[nodiscard]] std::optional<std::string> ReadMarker();
    [[nodiscard]] bool ReadHeader(GameSaveHeader& header);
    [[nodiscard]] bool ReadBytes(std::span<std::byte> bytes);
    [[nodiscard]] bool ReadConfig(BinaryXmlConfig& config);
    // Supplies the remaining bytes to an existing bounded prefix parser and
    // advances only by its reported nonzero length. On rejection/exception the
    // cursor returns to the section start. Parser-owned state is not rolled back.
    // The supplied span is transient and must not be retained by the parser.
    using PrefixParser = std::function<bool(std::span<const std::uint8_t>, std::size_t&)>;
    [[nodiscard]] bool ReadPrefix(const PrefixParser& parse,
                                 std::string_view error = "invalid save prefix");
    [[nodiscard]] std::uint64_t Position() const noexcept { return position_; }
    [[nodiscard]] std::uint64_t Remaining() const noexcept { return size_ - position_; }
    [[nodiscard]] bool IsOpen() const noexcept { return file_.is_open(); }
    [[nodiscard]] const std::string& LastError() const noexcept { return last_error_; }

private:
    bool Seek(std::uint64_t position);
    [[nodiscard]] std::uint32_t InvokeVirtual(
        std::uint32_t stream, std::uint32_t slot_offset,
        std::uintptr_t buffer, std::uint32_t bytes) override;
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

    std::ifstream file_;
    std::uint64_t size_ = 0;
    std::uint64_t position_ = 0;
    std::vector<char> text_storage_;
    std::string last_error_;
};

}  // namespace pal4
