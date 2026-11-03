#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace pal4::portable {

struct Vertex {
    float position[3], uv0[2], uv1[2], color[4];
};
struct Draw {
    std::uint32_t first, count;
    std::int32_t diffuse, lightmap;
    float color[4];
    std::array<std::string,8> uv_animations{};
};
using UvTransform=std::array<float,6>;
inline constexpr UvTransform kIdentityUv{1,0,0,1,0,0};
struct DrawAppearance {
    bool visible{};
    std::array<float,4> color{};
    std::array<UvTransform,2> uv{kIdentityUv,kIdentityUv};
    std::optional<float> sort_distance{};
};
enum class TextureFilter {linear, trilinear};
enum class TextureAddress {repeat, clamp};
struct Texture {
    std::uint32_t width{}, height{};
    std::vector<std::uint8_t> rgba;
    bool alpha{};
    TextureFilter filter{TextureFilter::trilinear};
    TextureAddress address{TextureAddress::repeat};
};
struct Scene {
    std::array<float, 3> minimum{}, maximum{}, saturation{};
    float brightness{};
    std::vector<Vertex> vertices;
    std::vector<Draw> draws;
    // Populated by loaders preserving original atomic draw boundaries. Entries
    // are baked into the same world space as vertices, one center per draw.
    std::vector<std::array<float,3>> atomic_centers;
    std::vector<Texture> textures;
};

// P4SC is a little-endian transport format, never a serialized host object.
// No GPU or operating-system dependencies belong in this reader.
Scene ReadScene(std::istream& stream);

struct OrbitCamera {
    std::array<float, 3> target{};
    float yaw{0.7f}, pitch{0.45f}, distance{1}, scale{1};
    void Fit(const Scene& scene);
    void Orbit(float horizontal, float vertical);
    void Zoom(float logarithmic_delta);
    std::array<float, 16> Matrix(float aspect) const;
};

}  // namespace pal4::portable
