#include "pal4/npc_manager_closest_finder.h"
#include "pal4/monster_manager_tree_cursor.h"

#include <bit>

namespace pal4 {

std::uint32_t NpcManagerClosestRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void NpcManagerClosestRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
bool NpcManagerClosestRuntimeStub::GetPositionAndData(
    std::uint32_t, std::uint32_t, std::uint32_t&) { return false; }
std::uint32_t NpcManagerClosestRuntimeStub::CountClumpMaterials(
    std::uint32_t) { return 0u; }
std::uint32_t NpcManagerClosestRuntimeStub::AdvanceMissingMaterials(
    std::uint32_t, std::uint32_t header) { return header; }
double NpcManagerClosestRuntimeStub::ApproximateMagnitude(
    float, float, float) { return 0.0f; }

std::uint32_t NpcManagerClosestFinder::Find(
    std::uint32_t manager, std::uint32_t reference,
    std::uint32_t data_bits, bool mark_selected,
    const NpcManagerTreeGlobals& globals,
    INpcManagerClosestRuntime& runtime) {
    const auto header = runtime.ReadDword(manager + 0x2cu);
    auto node = runtime.ReadDword(header);
    auto best_bits = 0x7f7fffffu;
    const auto bypass_gate = data_bits != 0u;
    std::uint32_t selected{};
    while (node != header) {
        const auto npc = runtime.ReadDword(node + 0x1cu);
        const auto record = runtime.ReadDword(npc + 0x654u);
        runtime.WriteDword(npc + 0x200u, 0u);
        const auto eligible = bypass_gate ||
            (runtime.ReadDword(npc + 0x1f0u) != 0u &&
             (record == 0u ||
              runtime.ReadDword(record + 0x40u) != 0u));
        if (eligible && runtime.GetPositionAndData(
                            npc, reference, data_bits) &&
            std::bit_cast<float>(data_bits) <
                std::bit_cast<float>(best_bits)) {
            best_bits = data_bits;
            selected = npc;
        }
        node = MonsterManagerTreeCursor::Successor(
            node, globals.nil_address,
            [&runtime](std::uint32_t address) {
                return runtime.ReadDword(address);
            }).next;
    }
    if (mark_selected && selected != 0u)
        runtime.WriteDword(selected + 0x200u, 1u);
    return selected;
}

namespace {
// IDA 43CF10：共用候选筛选；false 保留旧树的缺模型前进分支。
bool ConsiderNpc(std::uint32_t npc, std::uint32_t coordinate,
                 float& best, std::uint32_t& selected,
                 INpcManagerClosestRuntime& runtime) {
    const auto record = runtime.ReadDword(npc + 0x654u);
    const auto clump = runtime.ReadDword(npc + 0x14u);
    runtime.WriteDword(npc + 0x200u, 0u);
    if (runtime.CountClumpMaterials(clump) == 0u) return false;

    const auto active = runtime.ReadDword(npc + 0x1f0u) != 0u;
    const auto eligible = active &&
        (record == 0u || runtime.ReadDword(record + 0x40u) != 0u);
    if (eligible) {
        const auto x = std::bit_cast<float>(runtime.ReadDword(npc + 0xacu)) -
            std::bit_cast<float>(runtime.ReadDword(coordinate));
        const auto y = std::bit_cast<float>(runtime.ReadDword(npc + 0xb0u)) -
            std::bit_cast<float>(runtime.ReadDword(coordinate + 4u));
        const auto z = std::bit_cast<float>(runtime.ReadDword(npc + 0xb4u)) -
            std::bit_cast<float>(runtime.ReadDword(coordinate + 8u));
        const auto distance = runtime.ApproximateMagnitude(x, y, z);
        const auto current_record = runtime.ReadDword(npc + 0x654u);
        const auto radius = std::bit_cast<float>(
            runtime.ReadDword(current_record + 0x44u));
        if (distance <= radius && distance < best) {
            best = static_cast<float>(distance);
            selected = npc;
        }
    }
    return true;
}
}  // namespace

std::uint32_t NpcManagerClosestFinder::FindByCoordinateList(
    std::span<const std::uint32_t> candidates, std::uint32_t coordinate,
    bool mark_selected, INpcManagerClosestRuntime& runtime) {
    auto best = std::bit_cast<float>(0x7f7fffffu);
    std::uint32_t selected{};
    for (const auto npc : candidates)
        (void)ConsiderNpc(npc, coordinate, best, selected, runtime);
    if (mark_selected && selected != 0u)
        runtime.WriteDword(selected + 0x200u, 1u);
    return selected;
}

std::uint32_t NpcManagerClosestFinder::FindByCoordinate(
    std::uint32_t manager, std::uint32_t coordinate,
    bool mark_selected, const NpcManagerTreeGlobals& globals,
    INpcManagerClosestRuntime& runtime) {
    const auto header = runtime.ReadDword(manager + 0x2cu);
    auto node = runtime.ReadDword(header);
    auto best = std::bit_cast<float>(0x7f7fffffu);
    std::uint32_t selected{};
    while (node != header) {
        const auto npc = runtime.ReadDword(node + 0x1cu);
        if (!ConsiderNpc(npc, coordinate, best, selected, runtime)) {
            node = runtime.AdvanceMissingMaterials(node, header);
            continue;
        }
        node = MonsterManagerTreeCursor::Successor(
            node, globals.nil_address,
            [&runtime](std::uint32_t address) {
                return runtime.ReadDword(address);
            }).next;
    }
    if (mark_selected && selected != 0u)
        runtime.WriteDword(selected + 0x200u, 1u);
    return selected;
}

}  // namespace pal4
