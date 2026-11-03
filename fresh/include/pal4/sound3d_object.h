#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct Sound3DObjectState final {
    std::array<std::uint32_t, 23> words{};
};
static_assert(sizeof(Sound3DObjectState) == 0x5cu);

class ISound3DObjectRuntime {
public:
    virtual ~ISound3DObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ConstructorStackByte() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeText(std::uint32_t allocation) = 0;
    // Host adapters can retain the path in std::string instead of VC6 storage.
    virtual void ReleaseSoundPath(Sound3DObjectState& state);
    virtual void FreeObject(Sound3DObjectState& state) = 0;
    [[nodiscard]] virtual std::uint8_t StartPlayback(
        Sound3DObjectState& state, std::uint32_t facet_offset) = 0;
};

class Sound3DObjectRuntimeStub final : public ISound3DObjectRuntime {
public:
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint8_t ConstructorStackByte() override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void FreeText(std::uint32_t) override {}
    void FreeObject(Sound3DObjectState&) override {}
    [[nodiscard]] std::uint8_t StartPlayback(
        Sound3DObjectState&, std::uint32_t) override { return 0u; }
};

class Sound3DObject final {
public:
    // IDA 0x616b20; factory 0x63e270 allocates 0x5c bytes.
    static Sound3DObjectState& Construct(
        Sound3DObjectState& state, ISound3DObjectRuntime& runtime);
    // IDA 0x616be0.
    [[nodiscard]] static std::uintptr_t DeletingDestroy(
        Sound3DObjectState& state, std::uint32_t flags,
        ISound3DObjectRuntime& runtime);
    // IDA 0x616c00.
    static void Destroy(Sound3DObjectState& state,
                        ISound3DObjectRuntime& runtime);
    // IDA 0x617210.
    [[nodiscard]] static std::uint32_t ResourceProperty(
        const Sound3DObjectState& state);
    // IDA 0x617220.
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x617230. The native return is the low byte of the facet result.
    [[nodiscard]] static std::uint8_t SetEnabled(
        Sound3DObjectState& state, std::uint8_t enabled,
        ISound3DObjectRuntime& runtime);
};

}  // namespace pal4
