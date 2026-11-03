#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlayerManagerGlobalText {
    std::uint8_t initial_byte{};
    std::array<std::uint8_t, 3> padding{};
    std::uint32_t data{};
    std::uint32_t length{};
    std::uint32_t capacity{};
};
static_assert(sizeof(PlayerManagerGlobalText) == 16u);

class IPlayerManagerGlobalRuntime {
public:
    virtual ~IPlayerManagerGlobalRuntime() = default;
    virtual void SetErrorFlag(PlayerManagerGlobalText& text,
                              std::uint8_t flag) = 0;
    virtual void Reallocate(PlayerManagerGlobalText& text,
                            std::uint32_t length) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void WriteByte(std::uint32_t destination,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterExit(
        std::uint32_t callback_address) = 0;
};

class PlayerManagerGlobalRuntimeStub : public IPlayerManagerGlobalRuntime {
public:
    void SetErrorFlag(PlayerManagerGlobalText&, std::uint8_t) override;
    void Reallocate(PlayerManagerGlobalText&, std::uint32_t) override;
    void CopyText(std::uint32_t, std::string_view) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void Free(std::uint32_t) override;
    int RegisterExit(std::uint32_t) override;
};

class PlayerManagerGlobalData final {
public:
    static constexpr std::uint32_t kCleanupCallback = 0x441040u;
    static constexpr std::uint32_t kRotationCleanupCallback = 0x4411a0u;
    static constexpr std::uint32_t kTextAddress = 0x8e4390u;
    static constexpr std::uint32_t kRotationTextAddress = 0x8e437cu;
    static constexpr std::uint32_t kCleanupGuardAddress = 0x8e438cu;
    static constexpr std::string_view kName = "LeaderPos";
    static constexpr std::string_view kRotationName = "LeaderRot";

    // IDA 0x440f40. The callee reads a byte from uninitialized stack storage.
    [[nodiscard]] static int Initialize(
        PlayerManagerGlobalText& text, std::uint8_t incoming_stack_byte,
        IPlayerManagerGlobalRuntime& runtime);
    // IDA 0x440f50.
    static void InitializeName(PlayerManagerGlobalText& text,
        std::uint8_t incoming_stack_byte,
        IPlayerManagerGlobalRuntime& runtime);
    // IDA 0x441040.
    static void CleanupName(PlayerManagerGlobalText& text,
        std::uint8_t& cleanup_guard,
        IPlayerManagerGlobalRuntime& runtime);
    // IDA 0x4410a0.
    [[nodiscard]] static int InitializeRotation(
        PlayerManagerGlobalText& text, std::uint8_t incoming_stack_byte,
        IPlayerManagerGlobalRuntime& runtime);
    // IDA 0x4410b0: recovered code previously mislabeled as RTTI data.
    [[nodiscard]] static std::uint32_t InitializeRotationName(
        PlayerManagerGlobalText& text, std::uint8_t incoming_stack_byte,
        IPlayerManagerGlobalRuntime& runtime);
    // IDA 0x4411a0.
    static void CleanupRotationName(PlayerManagerGlobalText& text,
        std::uint8_t& cleanup_guard,
        IPlayerManagerGlobalRuntime& runtime);
};

}  // namespace pal4
