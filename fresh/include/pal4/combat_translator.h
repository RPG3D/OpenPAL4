#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatTranslatorRuntime {
public:
    virtual ~ICombatTranslatorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructBackend(
        std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint8_t LoadBackend(
        std::uint32_t backend, std::uint32_t input,
        std::uint32_t adapter_vtable) = 0;
    [[nodiscard]] virtual std::uint32_t FindNamedBackend(
        std::uint32_t backend, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ClearBackend(
        std::uint32_t backend) = 0;
    virtual void DestroyBackend(std::uint32_t backend) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
};

class CombatTranslator final {
public:
    static constexpr std::uint32_t kAdapterVtable = 0x84488cu;

    // IDA 0x574e10.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ICombatTranslatorRuntime& runtime);
    // IDA 0x574e70, a thunk to 0x574ed0.
    [[nodiscard]] static std::uint32_t DestructThunk(
        std::uint32_t object, ICombatTranslatorRuntime& runtime);
    // IDA 0x574e80.
    [[nodiscard]] static std::uint8_t Load(
        std::uint32_t object, std::uint32_t input,
        ICombatTranslatorRuntime& runtime);
    // IDA 0x574ed0.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ICombatTranslatorRuntime& runtime);
};

}  // namespace pal4
