#pragma once

#include <cstdint>

namespace pal4 {

enum class GameWorldFadeRemoval : std::uint8_t {
    ScriptOnRemove3,
    ScriptOnRemove5,
};

class IGameWorldFadeRuntime {
public:
    virtual ~IGameWorldFadeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SetMaterialFlags(
        std::uint32_t clump, std::uint32_t rgba) = 0;
    virtual void ScriptOnRemove(std::uint32_t world,
                                std::uint32_t object,
                                GameWorldFadeRemoval callback) = 0;
};

class GameWorldFadeRuntimeStub final : public IGameWorldFadeRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SetMaterialFlags(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void ScriptOnRemove(std::uint32_t, std::uint32_t,
                        GameWorldFadeRemoval) override {}
};

class GameWorldFadeQueue final {
public:
    static constexpr std::uint32_t kRecordBytes = 12u;

    // IDA 0x519e70: elapsed-time/material update and in-place record removal.
    [[nodiscard]] static std::uint32_t Advance(
        std::uint32_t world, float delta, IGameWorldFadeRuntime& runtime);
    // IDA 0x519fc0: fade-in queue at world+636/+640.
    [[nodiscard]] static std::uint32_t AdvanceIn(
        std::uint32_t world, float delta, IGameWorldFadeRuntime& runtime);
    // IDA 0x51a0b0: second fade-out queue at world+652/+656.
    [[nodiscard]] static std::uint32_t AdvanceOutSecondary(
        std::uint32_t world, float delta, IGameWorldFadeRuntime& runtime);
};

}  // namespace pal4
