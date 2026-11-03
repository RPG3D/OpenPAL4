#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/file_stream_lifecycle.h"

namespace pal4 {

class ITgaImageLoadRuntime : public IFileStreamReleaseRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(
        std::uint32_t source, std::string_view mode) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMetadata(
        std::uint32_t size, std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool LoadHeader(
        std::uint32_t stream, std::uint32_t metadata) = 0;
    virtual void CloseFile(std::uint32_t stream) = 0;
    virtual void NotifyError(std::uint32_t stream, std::uint32_t code) = 0;
    [[nodiscard]] virtual std::string_view ErrorMessage(
        std::uint32_t code) = 0;
    virtual void LogError(std::string_view file, std::uint32_t line,
                          std::string_view message) = 0;
    [[nodiscard]] virtual std::uint32_t CreateImage(
        std::uint16_t width, std::uint16_t height,
        std::uint32_t depth) = 0;
    virtual void AllocatePixels(std::uint32_t image) = 0;
};

class TgaImageLoadRuntimeStub final : public ITgaImageLoadRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t OpenFile(
        std::uint32_t, std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t AllocateMetadata(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] bool LoadHeader(std::uint32_t,
                                  std::uint32_t) override { return false; }
    void CloseFile(std::uint32_t) override {}
    void NotifyError(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::string_view ErrorMessage(
        std::uint32_t) override { return {}; }
    void LogError(std::string_view, std::uint32_t,
                  std::string_view) override {}
    [[nodiscard]] std::uint32_t CreateImage(
        std::uint16_t, std::uint16_t,
        std::uint32_t) override { return 0u; }
    void AllocatePixels(std::uint32_t) override {}
};

class TgaImageLoader final {
public:
    // IDA 0x5f8a00.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t source, ITgaImageLoadRuntime& runtime);
};

}  // namespace pal4
