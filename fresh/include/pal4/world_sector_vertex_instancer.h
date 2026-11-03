#pragma once

#include <cstdint>
#include <span>

namespace pal4 {

struct WorldSectorVertexElement {
    std::uint16_t stream;
    std::uint16_t offset;
    std::uint8_t type;
    std::uint8_t method;
    std::uint8_t usage;
    std::uint8_t usage_index;

    [[nodiscard]] bool operator==(const WorldSectorVertexElement&) const = default;
};
static_assert(sizeof(WorldSectorVertexElement) == 8u);

class IWorldSectorVertexInstancerRuntime {
public:
    virtual ~IWorldSectorVertexInstancerRuntime() = default;
    // Original reads the current world's flags through rwEngineInstance.
    [[nodiscard]] virtual std::uint32_t WorldFlags() = 0;
    [[nodiscard]] virtual std::uint32_t VertexCapsAddress() = 0;
    [[nodiscard]] virtual std::uint32_t WorldSectorUsageFlags(std::uint32_t sector) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint16_t ReadHalf(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteHalf(std::uint32_t address, std::uint16_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void CreateVertexDeclaration(
        std::span<const WorldSectorVertexElement> elements,
        std::uint32_t output_address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateVertexBuffer(
        std::uint32_t stride, std::uint32_t bytes,
        std::uint32_t buffer_address,
        std::uint32_t buffer_offset_address) = 0;
    [[nodiscard]] virtual std::uint32_t LockVertexBuffer(
        std::uint32_t buffer, std::uint32_t offset,
        std::uint32_t bytes, std::uint32_t flags) = 0;
    virtual void UnlockVertexBuffer(std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::uint32_t InstanceV3d(
        std::uint8_t type, std::uint32_t destination,
        std::uint32_t source, std::uint32_t count,
        std::uint32_t stride) = 0;
    [[nodiscard]] virtual std::uint32_t InstanceV3dComp(
        std::uint8_t type, std::uint32_t destination,
        std::uint32_t source, std::uint32_t count,
        std::uint32_t stride) = 0;
    [[nodiscard]] virtual std::uint32_t InstanceColor(
        std::uint32_t destination, std::uint32_t source,
        std::uint32_t count, std::uint32_t stride) = 0;
    [[nodiscard]] virtual std::uint32_t InstanceV2d(
        std::uint8_t type, std::uint32_t destination,
        std::uint32_t source, std::uint32_t count,
        std::uint32_t stride) = 0;
    [[nodiscard]] virtual std::uint32_t InstanceTangent(
        std::uint8_t type, std::uint32_t destination,
        std::uint32_t positions, std::uint32_t zero,
        std::uint32_t normals, std::uint32_t texcoords,
        std::uint32_t header, std::uint32_t stride) = 0;
};

class WorldSectorVertexInstancer final {
public:
    [[nodiscard]] static std::uint32_t Instance(
        std::uint32_t sector, std::uint32_t header,
        IWorldSectorVertexInstancerRuntime& runtime);
};

}  // namespace pal4
