#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct PalGameIVRegistration {
    std::uint8_t guard = 0;
    std::uint32_t instance = 0;
};

class IPalGameIVRuntime {
public:
    virtual ~IPalGameIVRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
    virtual void InitializeVariables(std::uint32_t object) = 0;
    virtual void ConstructBone(std::uint32_t bone) = 0;
    virtual void InitializeString(std::uint32_t string) = 0;
    virtual void ConstructDictionary(std::uint32_t dictionary) = 0;
    virtual void DestroyDictionary(std::uint32_t dictionary) = 0;
    virtual void CleanupString(std::uint32_t string) = 0;
    [[nodiscard]] virtual std::uint8_t ConstructorStackByte() = 0;
};

class PalGameIV final {
public:
    // IDA 51B573/520DB3/520E93/53EE42/53F5B1 use the object base.
    // Decompiler data[424] starts after the four-byte object header.
    static constexpr std::uint32_t kTextureDictionaryOffset = 0x1acu;

    [[nodiscard]] static std::uint32_t GetInstance(
        PalGameIVRegistration& registration, IPalGameIVRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(IPalGameIVRuntime& runtime);
    static void Destroy(std::uint32_t object, IPalGameIVRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t stack_byte,
        IPalGameIVRuntime& runtime);
    static void Cleanup(std::uint32_t object, IPalGameIVRuntime& runtime);
    static void Destruct(std::uint32_t object, IPalGameIVRuntime& runtime);
};

}  // namespace pal4
