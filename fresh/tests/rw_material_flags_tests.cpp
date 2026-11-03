#include "pal4/rw_material_flags.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IRwMaterialFlagsRuntime {
public:
    std::uint32_t ForAllAtomics(
        const std::uint32_t clump,
        const pal4::RwAtomicFlagsCallback callback,
        const std::uint32_t mode) override {
        ++calls;
        last_clump = clump;
        last_mode = mode;
        for (const auto atomic : atomics) {
            Check(callback(atomic, mode, *this) == atomic,
                  "atomic callback returns its input pointer");
        }
        return result;
    }
    std::uint32_t ForAllWorldMaterials(
        const std::uint32_t world,
        const pal4::RwMaterialFlagsCallback callback,
        const std::uint32_t mode) override {
        ++world_calls;
        last_world = world;
        last_world_mode = mode;
        for (const auto material : world_materials) {
            Check(callback(material, mode, *this) == material,
                  "world callback returns its material pointer");
        }
        return result;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        writes[address] = value;
        write_order.push_back(address);
    }

    std::uint32_t calls{};
    std::uint32_t last_clump{};
    std::uint32_t last_mode{};
    std::uint32_t world_calls{};
    std::uint32_t last_world{};
    std::uint32_t last_world_mode{};
    std::uint32_t result{0xfaceu};
    std::vector<std::uint32_t> atomics;
    std::vector<std::uint32_t> world_materials;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> writes;
    std::vector<std::uint32_t> write_order;
};

void ClumpModeMapping() {
    Runtime runtime;
    runtime.atomics = {0x2000u};
    runtime.words[0x2018u] = 0u;
    for (std::uint32_t mode = 0u; mode <= 5u; ++mode) {
        Check(pal4::RwMaterialFlags::SetClump(
                  0x1000u, mode, runtime) == runtime.result &&
                  runtime.calls == mode + 1u &&
                  runtime.last_clump == 0x1000u &&
                  runtime.last_mode == mode + 1u,
              "valid mode increments before RenderWare atomic traversal");
    }
    Check(pal4::RwMaterialFlags::SetClump(
              0x1000u, 6u, runtime) == 6u &&
              pal4::RwMaterialFlags::SetClump(
                  0x1000u, 0xffffffffu, runtime) == 0xffffffffu &&
              runtime.calls == 6u,
          "out-of-range modes return unchanged without traversal");
}

void AtomicMaterialTraversal() {
    Runtime runtime;
    constexpr auto atomic = 0x2000u;
    constexpr auto geometry = 0x3000u;
    constexpr auto material_list = 0x4000u;
    runtime.words[atomic + 24u] = 0u;
    Check(pal4::RwMaterialFlags::SetAtomic(
              atomic, 0x12345678u, runtime) == atomic &&
              runtime.writes.empty(),
          "atomic without geometry returns without material access");

    runtime.words[atomic + 24u] = geometry;
    runtime.words[geometry + 36u] = 0u;
    Check(pal4::RwMaterialFlags::SetAtomic(
              atomic, 1u, runtime) == atomic,
          "zero material count does not read the list");
    runtime.words[geometry + 36u] = 0xffffffffu;
    Check(pal4::RwMaterialFlags::SetAtomic(
              atomic, 1u, runtime) == atomic,
          "negative signed material count is skipped");

    runtime.words[geometry + 36u] = 3u;
    runtime.words[geometry + 32u] = material_list;
    runtime.words[material_list] = 0x5000u;
    runtime.words[material_list + 4u] = 0x5100u;
    runtime.words[material_list + 8u] = 0x5200u;
    runtime.words[0x5000u] = 0x6000u;
    runtime.words[0x5100u] = 0u;
    runtime.words[0x5200u] = 0x6200u;
    Check(pal4::RwMaterialFlags::SetAtomic(
              atomic, 0x12345678u, runtime) == atomic &&
              runtime.write_order ==
                  std::vector<std::uint32_t>{0x6050u, 0x6250u} &&
              runtime.writes[0x6050u] == 0x78u &&
              runtime.writes[0x6250u] == 0x78u,
          "materials receive low mode byte in geometry array order");
    Check(pal4::RwMaterialFlags::SetMaterial(
              0x5100u, 0x34u, runtime) == 0x5100u &&
              runtime.write_order.size() == 2u,
          "material with null target returns without writing");
}

void RenderWareStub() {
    pal4::RwMaterialFlagsRuntimeStub runtime;
    Check(runtime.ReadWord(0x1000u) == 0u &&
              pal4::RwMaterialFlags::SetClump(
                  0x1000u, 0u, runtime) == 0u &&
              pal4::RwMaterialFlags::SetWorld(
                  0x1000u, 0u, runtime) == 0u,
          "RenderWare traversal stubs are linkable before library integration");
}

void WorldModeMapping() {
    Runtime runtime;
    runtime.world_materials = {0x5000u, 0x5100u};
    runtime.words[0x5000u] = 0x6000u;
    runtime.words[0x5100u] = 0u;
    for (std::uint32_t mode = 0u; mode <= 5u; ++mode) {
        Check(pal4::RwMaterialFlags::SetWorld(
                  0x1000u, mode, runtime) == runtime.result &&
                  runtime.world_calls == mode + 1u &&
                  runtime.last_world == 0x1000u &&
                  runtime.last_world_mode == mode + 1u &&
                  runtime.writes[0x6050u] == mode + 1u,
              "valid world mode maps and reaches material callback");
    }
    Check(pal4::RwMaterialFlags::SetWorld(
              0x1000u, 6u, runtime) == 6u &&
              pal4::RwMaterialFlags::SetWorld(
                  0x1000u, 0xffffffffu, runtime) == 0xffffffffu &&
              runtime.world_calls == 6u,
          "invalid world modes skip RenderWare traversal");
}

}  // namespace

int main() {
    ClumpModeMapping();
    AtomicMaterialTraversal();
    RenderWareStub();
    WorldModeMapping();
    std::cout << "4 RW material flag test cases passed\n";
}
