#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class IGameDataRoleFieldNamesRuntime {
public:
    virtual ~IGameDataRoleFieldNamesRuntime() = default;
    [[nodiscard]] virtual std::uint8_t InitialSeed() = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t address,
                                            std::string_view text) = 0;
    virtual void AssignText(std::uint32_t address,
                            std::string_view text) = 0;
    virtual void DestroyText(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterAtexit(
        std::uint32_t callback_address) = 0;
};

class GameDataRoleFieldNames final {
public:
    [[nodiscard]] static std::optional<std::string_view> NameAt(
        std::uint32_t address);
    // IDA 0x4e76d0: initialize four RA_* texts and register cleanup.
    [[nodiscard]] static int InitializeAndRegister(
        IGameDataRoleFieldNamesRuntime& runtime);
    // IDA 0x4e76e0: initialize contiguous RA_NAME..RA_TEXTURE texts.
    static void Initialize(IGameDataRoleFieldNamesRuntime& runtime);
    // IDA 0x4e7840: destroy four texts in reverse order.
    static void Destroy(IGameDataRoleFieldNamesRuntime& runtime);
};

}  // namespace pal4
