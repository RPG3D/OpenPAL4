#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct GameDbFlagStaticState final {
    std::uint8_t initialized_byte = 0u;
    std::uint8_t cleanup_guard = 0u;
    std::uint32_t data = 0u;
    std::uint32_t length = 0u;
    std::uint32_t capacity = 0u;
};

class IGameDbFlagStaticRuntime {
public:
    virtual ~IGameDbFlagStaticRuntime() = default;
    [[nodiscard]] virtual std::uint8_t SetErrorFlag(
        GameDbFlagStaticState& state, std::uint8_t flag) = 0;
    virtual void Reallocate(GameDbFlagStaticState& state,
                            std::uint32_t length) = 0;
    virtual void CopyLiteral(std::uint32_t destination,
                             std::string_view text) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterCleanup() = 0;
};

class GameDbFlagStatic final {
public:
    static constexpr std::string_view kText = "GAME_DB_FLAG";
    // IDA 0x651b70: construct the static text, then register its cleanup.
    [[nodiscard]] static int Register(GameDbFlagStaticState& state,
                                      std::uint8_t initial_byte,
                                      IGameDbFlagStaticRuntime& runtime);
    // IDA 0x651b80: construct the global text buffer.
    [[nodiscard]] static std::uint8_t Initialize(
        GameDbFlagStaticState& state, std::uint8_t initial_byte,
        IGameDbFlagStaticRuntime& runtime);
    // IDA 0x651c70: once-only reference-counted cleanup.
    static void Deinitialize(GameDbFlagStaticState& state,
                             IGameDbFlagStaticRuntime& runtime);
};

}  // namespace pal4
