#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptGlobalTextRuntime {
public:
    virtual ~IScriptGlobalTextRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t record, std::uint8_t value) = 0;
    virtual void SetExceptionMessage(std::uint32_t record,
                                     std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeCString(
        std::uint32_t record, std::string_view text,
        std::uint8_t seed) = 0;
    [[nodiscard]] virtual std::uint8_t AllocateBuffer(
        std::uint32_t record, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::string_view text) = 0;
    [[nodiscard]] virtual int RegisterAtExit(std::uint32_t callback) = 0;
    virtual void DestroyRange(std::uint32_t first, std::uint32_t stride,
                              std::uint32_t count,
                              std::uint32_t destructor) = 0;
};

class ScriptGlobalTextCatalog final {
public:
    static constexpr std::uint32_t kPrimaryBase = 0x8fa0b8u;
    static constexpr std::uint32_t kSecondaryBase = 0x8fa060u;
    static constexpr std::uint32_t kPrimaryGuard = 0x8fa0b0u;
    static constexpr std::uint32_t kSecondaryGuard = 0x8fa058u;
    static constexpr std::uint32_t kTextDestructor = 0x51f9b0u;

    // IDA 0x56ec50/0x56ec60/0x56efe0.
    [[nodiscard]] static int RegisterPrimary(
        std::uint8_t seed, IScriptGlobalTextRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializePrimary(
        std::uint8_t seed, IScriptGlobalTextRuntime& runtime);
    static void CleanupPrimary(IScriptGlobalTextRuntime& runtime);

    // IDA 0x56f010/0x56f020/0x56f1b0.
    [[nodiscard]] static int RegisterSecondary(
        std::uint8_t seed, IScriptGlobalTextRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeSecondary(
        std::uint8_t seed, IScriptGlobalTextRuntime& runtime);
    static void CleanupSecondary(IScriptGlobalTextRuntime& runtime);
};

}  // namespace pal4
