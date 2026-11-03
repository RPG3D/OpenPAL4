#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

enum class ParameterFieldGroup {
    Attack,
    Product,
    Progression,
    Special,
    Elements,
    Materials,
};

class IGameDataParameterFieldNamesRuntime {
public:
    virtual ~IGameDataParameterFieldNamesRuntime() = default;
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

class GameDataParameterFieldNames final {
public:
    [[nodiscard]] static std::optional<std::string_view> NameAt(std::uint32_t address);
    // IDA 0x4e0fd0-0x4e1c20: six initializer, destructor and CRT triples.
    [[nodiscard]] static int InitializeAndRegister(
        ParameterFieldGroup group, IGameDataParameterFieldNamesRuntime& runtime);
    static void Initialize(ParameterFieldGroup group,
                           IGameDataParameterFieldNamesRuntime& runtime);
    static void Destroy(ParameterFieldGroup group,
                        IGameDataParameterFieldNamesRuntime& runtime);
};

}  // namespace pal4
