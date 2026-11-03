#pragma once

#include <cstdint>

namespace pal4 {

class IWorldMapFlyCallbackRuntime {
public:
    virtual ~IWorldMapFlyCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    virtual void SetErrorFlag(std::uint32_t text, std::uint8_t value) = 0;
    virtual void EraseText(std::uint32_t text, std::uint32_t start,
                           std::uint32_t count) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t text,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class WorldMapFlyCallback final {
public:
    static constexpr std::uint32_t kVtable = 0x843cd4u;
    static constexpr std::uint32_t kBaseVtable = 0x843b40u;

    // Source is the 16-byte, by-value string argument at the call site.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t function, std::uint32_t adjustment,
        std::uint32_t source, IWorldMapFlyCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IWorldMapFlyCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        IWorldMapFlyCallbackRuntime& runtime);
};

}  // namespace pal4
