#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

struct GameLoadArgument {
    // The native callback reads this pointer from each argument record at +4.
    const char* value;
};

class IGameArenaLoadRuntime {
public:
    virtual ~IGameArenaLoadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeXmlDocument() = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> FindAttribute(
        std::uint32_t document, std::string_view key) = 0;
    virtual void SetInt(std::uint32_t attribute, std::int32_t value) = 0;
    virtual void SetString(std::uint32_t attribute,
                           std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeIntAttribute(
        std::string_view key, std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeStringAttribute(
        std::string_view key, std::string_view value) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void InitializeStateAndUi(std::uint32_t game,
                                      std::uint32_t state,
                                      std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void ProcessActorInput(std::uint32_t actor) = 0;
    virtual void ProcessActorInputAndClearVector(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t InputManager() = 0;
    virtual void InitializeInputState(std::uint32_t input) = 0;
    [[nodiscard]] virtual std::uint32_t Milliseconds() = 0;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    [[nodiscard]] virtual std::int32_t DestroyXmlDocument(
        std::uint32_t document) = 0;
};

class GameArenaLoadHandler final {
public:
    // IDA 0x5d30e0, giArenaLoad callback.
    [[nodiscard]] static std::int32_t Load(
        GameLoadArgument arena, GameLoadArgument area,
        GameLoadArgument area_resource, IGameArenaLoadRuntime& runtime);
};

}  // namespace pal4
