#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class PropertyFieldGroup { Core, Extra };

class IGameDataPropertyFieldNamesRuntime {
public:
    virtual ~IGameDataPropertyFieldNamesRuntime() = default;
    [[nodiscard]] virtual std::uint8_t InitialSeed() = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ResetText(std::uint32_t address) = 0;
    virtual void AssignText(std::uint32_t address,
                            std::string_view text) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t address,
                                            std::string_view text) = 0;
    virtual void DestroyText(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterAtexit(
        std::uint32_t callback_address) = 0;
};

class GameDataPropertyFieldNames final {
public:
    // IDA 0x4e4970-0x4e4dd0: two groups of property field-name buffers.
    [[nodiscard]] static int InitializeAndRegister(
        PropertyFieldGroup group,
        IGameDataPropertyFieldNamesRuntime& runtime);
    static void Initialize(PropertyFieldGroup group,
                           IGameDataPropertyFieldNamesRuntime& runtime);
    static void Destroy(PropertyFieldGroup group,
                        IGameDataPropertyFieldNamesRuntime& runtime);
};

}  // namespace pal4
