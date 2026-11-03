#include "pal4/game_world_material_operations.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldMaterialFlagRuntime {
public:
    std::uint32_t GeometryOfAtomic(std::uint32_t atomic) override {
        Check(atomic == 0xa000u, "atomic identifies geometry owner");
        calls.push_back("geometry");
        return 0xb000u;
    }
    std::uint32_t GeometryFlags(std::uint32_t geometry) override {
        Check(geometry == 0xb000u, "geometry flags read from atomic geometry");
        calls.push_back("flags");
        return geometry_flags;
    }
    void SetGeometryFlags(std::uint32_t geometry,
                          std::uint32_t flags) override {
        Check(geometry == 0xb000u, "geometry flags written to same geometry");
        calls.push_back("set_flags");
        geometry_flags = flags;
    }
    std::int32_t MaterialCount(std::uint32_t geometry) override {
        Check(geometry == 0xb000u, "material count from same geometry");
        calls.push_back("count");
        return count;
    }
    std::uint32_t MaterialAt(std::uint32_t geometry,
                             std::int32_t index) override {
        Check(geometry == 0xb000u && index >= 0 && index < count,
              "material index remains within geometry count");
        calls.push_back("material");
        return 0xc000u + static_cast<std::uint32_t>(index) * 0x100u;
    }
    void SetMaterialFlags(std::uint32_t material,
                          std::uint32_t flags) override {
        Check(flags == 0x77u, "same flags reach every material");
        calls.push_back("set_material");
        materials.push_back(material);
    }
    std::uint32_t ForAllClumpAtomics(
        std::uint32_t clump, std::uint32_t flags) override {
        Check(clump == 0xd000u && flags == 0x77u,
              "clump traversal receives object and callback flag value");
        calls.push_back("traverse");
        return traverse_result;
    }

    std::uint32_t geometry_flags = 8u;
    std::int32_t count = 3;
    std::uint32_t traverse_result = 0x8888u;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> materials;
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::GameWorldMaterialOperations::SetAtomicFlags(
              0xa000u, 0x77u, runtime) == 0xa000u &&
              runtime.geometry_flags == 0x48u &&
              runtime.materials ==
                  std::vector<std::uint32_t>{0xc000u, 0xc100u, 0xc200u} &&
              runtime.calls == std::vector<std::string>{
                  "geometry", "flags", "count", "set_flags", "material",
                  "set_material", "material", "set_material", "material",
                  "set_material"},
          "geometry flag upgrade and material writes follow native order");

    runtime.calls.clear();
    runtime.materials.clear();
    runtime.geometry_flags = 0x40u;
    runtime.count = 0;
    Check(pal4::GameWorldMaterialOperations::SetAtomicFlags(
              0xa000u, 0x77u, runtime) == 0xa000u &&
              runtime.calls ==
                  std::vector<std::string>{"geometry", "flags", "count"},
          "no geometry upgrade or material writes without flag eight/count");
    runtime.calls.clear();
    runtime.geometry_flags = 0x48u;
    runtime.count = -1;
    Check(pal4::GameWorldMaterialOperations::SetAtomicFlags(
              0xa000u, 0x77u, runtime) == 0xa000u &&
              runtime.calls ==
                  std::vector<std::string>{"geometry", "flags", "count"},
          "already-upgraded geometry and negative material count are bypassed");

    runtime.calls.clear();
    Check(pal4::GameWorldMaterialOperations::SetClumpFlags(
              0xd000u, 0x77u, runtime) == 0x8888u &&
              runtime.calls == std::vector<std::string>{"traverse"},
          "clump wrapper forwards traversal result and callback flags");
}
