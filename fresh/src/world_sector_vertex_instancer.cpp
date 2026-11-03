#include "pal4/world_sector_vertex_instancer.h"

#include <vector>

namespace pal4 {

std::uint32_t WorldSectorVertexInstancer::Instance(
    const std::uint32_t sector, const std::uint32_t header,
    IWorldSectorVertexInstancerRuntime& runtime) {
    const auto caps = runtime.VertexCapsAddress();
    const auto flags = runtime.WorldFlags();
    const auto usage_flags = runtime.WorldSectorUsageFlags(sector);
    const auto vertex_count = runtime.ReadHalf(sector + 130u);
    runtime.WriteWord(header + 60u, vertex_count);

    const bool compact = (runtime.ReadByte(caps + 236u) & 0x80u) != 0u;
    std::vector<WorldSectorVertexElement> elements;
    elements.reserve(7u);
    elements.push_back({0u, 0u, 2u, 0u, 0u, 0u});
    std::uint16_t offset = 12u;
    std::uint32_t stride = 12u;
    std::uint16_t attributes = 2u;
    runtime.WriteWord(header + 24u, stride);
    runtime.WriteHalf(header + 28u, attributes);

    if ((flags & 0x10u) != 0u) {
        elements.push_back({0u, offset,
                            static_cast<std::uint8_t>(compact ? 14u : 2u),
                            0u, 3u, 0u});
        stride = compact ? 16u : 24u;
        offset = static_cast<std::uint16_t>(stride);
        attributes = 6u;
        runtime.WriteWord(header + 24u, stride);
        runtime.WriteHalf(header + 28u, attributes);
    }
    if ((flags & 8u) != 0u) {
        elements.push_back({0u, offset, 4u, 0u, 10u, 0u});
        stride += 4u;
        offset = static_cast<std::uint16_t>(offset + 4u);
        attributes = static_cast<std::uint16_t>(attributes | 8u);
        runtime.WriteWord(header + 24u, stride);
        runtime.WriteHalf(header + 28u, attributes);
    }
    if ((flags & 0x84u) != 0u) {
        elements.push_back({0u, offset, 1u, 0u, 5u, 0u});
        stride += 8u;
        offset = static_cast<std::uint16_t>(offset + 8u);
        attributes = static_cast<std::uint16_t>(attributes | 0x10u);
        runtime.WriteWord(header + 24u, stride);
        runtime.WriteHalf(header + 28u, attributes);
    }
    const auto second_texcoords = runtime.ReadWord(sector + 20u);
    if (second_texcoords != 0u) {
        elements.push_back({0u, offset, 1u, 0u, 5u, 1u});
        stride += 8u;
        offset = static_cast<std::uint16_t>(offset + 8u);
        attributes = static_cast<std::uint16_t>(attributes | 0x20u);
        runtime.WriteWord(header + 24u, stride);
        runtime.WriteHalf(header + 28u, attributes);
    }
    if ((usage_flags & 0x10000u) != 0u) {
        elements.push_back({0u, offset,
                            static_cast<std::uint8_t>(compact ? 14u : 2u),
                            0u, 6u, 0u});
        stride += compact ? 4u : 12u;
        runtime.WriteWord(header + 24u, stride);
    }
    elements.push_back({255u, 0u, 17u, 0u, 0u, 0u});
    runtime.CreateVertexDeclaration(elements, header + 52u);

    const auto bytes = stride * vertex_count;
    runtime.WriteByte(header + 30u, 1u);
    const auto created = runtime.CreateVertexBuffer(
        stride, bytes, header + 16u, header + 20u);
    if (created == 0u) return created;

    runtime.WriteWord(header + 48u, 0u);
    const auto mesh_count = runtime.ReadWord(header + 4u);
    if (mesh_count == 0u) return 0u;
    const auto buffer_offset = runtime.ReadWord(header + 20u);
    for (std::uint32_t index = 0; index < mesh_count; ++index) {
        // Cursor points to RxD3D9InstanceData::minVert, not record start.
        // 53E79B reads minVert; 53E7A3 replaces baseIndex (no accumulation).
        const auto mesh = header + 68u + 36u * index;
        runtime.WriteWord(mesh + 16u,
                          runtime.ReadWord(mesh) +
                              buffer_offset / stride);
    }

    const auto buffer = runtime.ReadWord(header + 16u);
    const auto mapped = runtime.LockVertexBuffer(
        buffer, buffer_offset, bytes, 0u);
    std::uint32_t written = runtime.InstanceV3d(
        elements[0].type, mapped, runtime.ReadWord(sector + 8u),
        vertex_count, stride);
    std::size_t element_index = 1u;
    if ((flags & 0x10u) != 0u) {
        written += runtime.InstanceV3dComp(
            elements[element_index++].type, mapped + written,
            runtime.ReadWord(sector + 12u), vertex_count, stride);
    }
    if ((flags & 8u) != 0u) {
        const auto colors = runtime.ReadWord(sector + 48u);
        for (std::uint32_t index = 0; index < mesh_count; ++index) {
            const auto mesh = header + 68u + 36u * index;
            const auto start = runtime.ReadWord(mesh);
            const auto result = runtime.InstanceColor(
                mapped + stride * start + written,
                colors + 4u * start, runtime.ReadWord(mesh + 20u), stride);
            runtime.WriteWord(mesh + 8u, result);
        }
        written += 4u;
        ++element_index;
    } else {
        for (std::uint32_t index = 0; index < mesh_count; ++index)
            runtime.WriteWord(header + 76u + 36u * index, 0u);
    }
    if ((flags & 0x84u) != 0u) {
        written += runtime.InstanceV2d(
            elements[element_index++].type, mapped + written,
            runtime.ReadWord(sector + 16u), vertex_count, stride);
    }
    if (second_texcoords != 0u) {
        written += runtime.InstanceV2d(
            elements[element_index++].type, mapped + written,
            second_texcoords, vertex_count, stride);
    }
    if ((usage_flags & 0x10000u) != 0u) {
        written += runtime.InstanceTangent(
            elements[element_index].type, mapped + written,
            runtime.ReadWord(sector + 8u), 0u,
            runtime.ReadWord(sector + 12u),
            runtime.ReadWord(sector + ((flags & 0x80u) != 0u ? 20u : 16u)),
            header, stride);
    }
    if (written != stride) return 0u;
    runtime.UnlockVertexBuffer(buffer);
    return 1u;
}

}  // namespace pal4
