#pragma once

#include <cstdint>

namespace pal4 {

enum class EffectRegistrationTree : std::uint8_t { Clumps, Atomics };

class IEffectSystemRegistrationRuntime {
public:
    virtual ~IEffectSystemRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FindLowerBound(
        EffectRegistrationTree kind, std::uint32_t tree,
        std::uint32_t key) = 0;
    [[nodiscard]] virtual bool InsertUnique(
        EffectRegistrationTree kind, std::uint32_t tree,
        std::uint32_t key, std::uint32_t saved_callback) = 0;
    [[nodiscard]] virtual std::uint32_t Erase(
        EffectRegistrationTree kind, std::uint32_t tree,
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t EffectCallbackAddress() = 0;
    [[nodiscard]] virtual std::uint32_t DefaultCallbackAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessAllAtomics(
        std::uint32_t clump, std::uint8_t enabled) = 0;
};

class EffectSystemRegistration final {
public:
    // IDA 0x63d000/0x63d160: atomic callback ownership in tree +28.
    [[nodiscard]] static std::uint8_t AddEffectToTree(
        std::uint32_t auxiliary, std::uint32_t atomic,
        IEffectSystemRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint8_t RemoveEffectFromTree(
        std::uint32_t auxiliary, std::uint32_t atomic,
        IEffectSystemRegistrationRuntime& runtime);
    // IDA 0x63d1e0/0x63d2f0: clump registration in tree +12.
    [[nodiscard]] static bool HandleEffectAddition(
        std::uint32_t auxiliary, std::uint32_t clump,
        IEffectSystemRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint8_t RemoveClumpAndProcessAtomics(
        std::uint32_t auxiliary, std::uint32_t clump,
        IEffectSystemRegistrationRuntime& runtime);
};

}  // namespace pal4
