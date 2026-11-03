#pragma once

#include <cstdint>

namespace pal4 {

class IIdGeneratorRegistryRuntime {
public:
    virtual ~IIdGeneratorRegistryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeRegistry(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class IdGeneratorRegistry final {
public:
    static constexpr std::uint32_t kNameAddress = 0x8a76d8u;
    static constexpr std::uint32_t kFactoryAddress = 0x4c5e10u;

    // IDA 0x4c5c40: jump thunk to Register.
    [[nodiscard]] static std::uint32_t RegisterViaThunk(
        std::uint32_t object, IIdGeneratorRegistryRuntime& runtime);
    // IDA 0x4c5c50: append the palIdGenerator registration pair.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t object, IIdGeneratorRegistryRuntime& runtime);
    // IDA 0x4c5e10: create an eight-byte factory object, or return zero.
    [[nodiscard]] static std::uint32_t CreateFactoryObject(
        IIdGeneratorRegistryRuntime& runtime);
    // IDA 0x4c5e30: signed size of the eight-byte pair array.
    [[nodiscard]] static std::int32_t Size(
        std::uint32_t registry, IIdGeneratorRegistryRuntime& runtime);
};

}  // namespace pal4
