#pragma once
#include "pal4/portable/scene.h"
#include <optional>
namespace pal4::portable {
struct CylinderShape {
    int sides{8},arc_degrees{360};
    float height{},top_radius{},bottom_radius{},border_width{};
    bool bordered{},alternate_uv{},centered{};
};
struct CylinderMesh {
    Scene scene;
    std::uint32_t atomics{},source_vertices{},source_triangles{};
};
// 复用原GeoCylinder生成器；原几何/材质API边界输出公共CPU网格。
CylinderMesh BuildCylinder(const CylinderShape& shape,std::optional<Texture> texture={});
}
