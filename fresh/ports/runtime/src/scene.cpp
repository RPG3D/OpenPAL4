#include "pal4/portable/scene.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <istream>
#include <stdexcept>

namespace pal4::portable {
namespace {
constexpr std::uint64_t kBudget = 384ull * 1024 * 1024;
struct SerializedDraw {
    std::uint32_t first, count;
    std::int32_t diffuse, lightmap;
    float color[4];
};
static_assert(sizeof(SerializedDraw) == 32);
void Read(std::istream& stream, void* target, std::size_t bytes) {
    if (!stream.read(static_cast<char*>(target), static_cast<std::streamsize>(bytes)))
        throw std::runtime_error("truncated scene package");
}
template<class T> T Read(std::istream& stream) {
    T value{};
    Read(stream, &value, sizeof value);
    return value;
}
void Finite(float value) {
    if (!std::isfinite(value)) throw std::runtime_error("non-finite scene value");
}
}

Scene ReadScene(std::istream& stream) {
    static_assert(std::endian::native == std::endian::little);
    static_assert(sizeof(Vertex) == 44);
    char magic[4]{};
    Read(stream, magic, sizeof magic);
    if (std::memcmp(magic, "P4SC", 4) || Read<std::uint32_t>(stream) != 1)
        throw std::runtime_error("unsupported scene package");
    const auto draw_count = Read<std::uint32_t>(stream);
    const auto vertex_count = Read<std::uint32_t>(stream);
    if (!draw_count || draw_count > 100000 || !vertex_count || vertex_count > 3000000)
        throw std::runtime_error("invalid geometry count");
    Scene scene;
    Read(stream, scene.minimum.data(), 12);
    Read(stream, scene.maximum.data(), 12);
    Read(stream, scene.saturation.data(), 12);
    scene.brightness = Read<float>(stream);
    Finite(scene.brightness);
    for (int axis = 0; axis < 3; ++axis) {
        Finite(scene.minimum[axis]); Finite(scene.maximum[axis]); Finite(scene.saturation[axis]);
        if (scene.minimum[axis] > scene.maximum[axis])
            throw std::runtime_error("inverted scene bounds");
    }
    scene.draws.resize(draw_count);
    scene.vertices.resize(vertex_count);
    for (auto& draw : scene.draws) {
        const auto stored = Read<SerializedDraw>(stream);
        draw.first = stored.first;
        draw.count = stored.count;
        draw.diffuse = stored.diffuse;
        draw.lightmap = stored.lightmap;
        std::copy(std::begin(stored.color), std::end(stored.color), draw.color);
    }
    Read(stream, scene.vertices.data(), scene.vertices.size() * sizeof(Vertex));
    for (const auto& vertex : scene.vertices) {
        for (float value : vertex.position) Finite(value);
        for (float value : vertex.uv0) Finite(value);
        for (float value : vertex.uv1) Finite(value);
        for (float value : vertex.color) Finite(value);
    }
    const auto texture_count = Read<std::uint32_t>(stream);
    if (texture_count > 4096) throw std::runtime_error("invalid texture count");
    std::uint64_t bytes_used = std::uint64_t(vertex_count) * sizeof(Vertex) +
                               std::uint64_t(draw_count) * sizeof(Draw);
    scene.textures.resize(texture_count);
    for (auto& texture : scene.textures) {
        texture.width = Read<std::uint32_t>(stream);
        texture.height = Read<std::uint32_t>(stream);
        const auto bytes = Read<std::uint32_t>(stream);
        const auto expected = std::uint64_t(texture.width) * texture.height * 4;
        if (!texture.width || !texture.height || texture.width > 4096 || texture.height > 4096 ||
            bytes != expected || expected > 64ull * 1024 * 1024 || bytes_used + expected > kBudget)
            throw std::runtime_error("invalid texture size or scene memory budget");
        bytes_used += expected;
        texture.rgba.resize(bytes);
        Read(stream, texture.rgba.data(), bytes);
        for (std::size_t offset = 3; offset < bytes; offset += 4)
            texture.alpha |= texture.rgba[offset] < 255;
    }
    for (const auto& draw : scene.draws) {
        if (!draw.count || draw.count % 3 || draw.first > vertex_count ||
            draw.count > vertex_count - draw.first || draw.diffuse < -1 || draw.lightmap < -1 ||
            draw.diffuse >= static_cast<std::int32_t>(texture_count) ||
            draw.lightmap >= static_cast<std::int32_t>(texture_count))
            throw std::runtime_error("invalid draw range or texture reference");
        for (float value : draw.color) Finite(value);
    }
    if (stream.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("scene package has trailing bytes");
    return scene;
}
}  // namespace pal4::portable
