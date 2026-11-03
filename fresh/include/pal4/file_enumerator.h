#pragma once

#include <cstdint>

namespace pal4 {

class IFileEnumeratorRuntime {
public:
    virtual ~IFileEnumeratorRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CopyString(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t FindFirst(
        std::uint32_t pattern, std::uint32_t find_data) = 0;
    [[nodiscard]] virtual std::uint32_t FindNext(
        std::uint32_t handle, std::uint32_t find_data) = 0;
    [[nodiscard]] virtual std::uint32_t FindClose(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual bool FullPath(std::uint32_t destination,
                                        std::uint32_t pattern,
                                        std::uint32_t capacity) = 0;
    [[nodiscard]] virtual std::uint32_t FindLastCharacter(
        std::uint32_t string, std::uint8_t character) = 0;
};

class FileEnumeratorRuntimeStub final : public IFileEnumeratorRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void CopyString(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FindFirst(
        std::uint32_t, std::uint32_t) override { return 0xffffffffu; }
    [[nodiscard]] std::uint32_t FindNext(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FindClose(std::uint32_t) override { return 0u; }
    [[nodiscard]] bool FullPath(std::uint32_t, std::uint32_t,
                                std::uint32_t) override { return false; }
    [[nodiscard]] std::uint32_t FindLastCharacter(
        std::uint32_t, std::uint8_t) override { return 0u; }
};

class FileEnumerator final {
public:
    // IDA 0x5fa3d0.
    [[nodiscard]] static std::uint32_t ClearStrings(
        std::uint32_t object, IFileEnumeratorRuntime& runtime);
    // IDA 0x5fa410.
    [[nodiscard]] static std::uint8_t IsDone(
        std::uint32_t object, IFileEnumeratorRuntime& runtime);
    // IDA 0x5fa420.
    [[nodiscard]] static std::uint32_t Close(
        std::uint32_t object, IFileEnumeratorRuntime& runtime);
    // IDA 0x5fa450.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t pattern,
        IFileEnumeratorRuntime& runtime);
    // IDA 0x5fa510.
    [[nodiscard]] static std::uint32_t Next(
        std::uint32_t object, IFileEnumeratorRuntime& runtime);
    // IDA 0x5fa530.
    [[nodiscard]] static bool IsSpecialDirectory(
        std::uint32_t object, IFileEnumeratorRuntime& runtime);
    // IDA 0x5fa580.
    [[nodiscard]] static bool IsDirectory(
        std::uint32_t object, IFileEnumeratorRuntime& runtime);
    // IDA 0x5fa590.
    [[nodiscard]] static std::uint32_t GetCurrentFileName(
        std::uint32_t object, IFileEnumeratorRuntime& runtime);
};

}  // namespace pal4
