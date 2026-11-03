#include "pal4/effect_system_group_draw.h"

#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

void DrawList(std::uint32_t system, std::uint32_t offset,
              std::uint32_t context,
              IEffectSystemGroupDrawRuntime& runtime) {
    const auto head = runtime.ReadWord(system + offset + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        runtime.DrawObject(runtime.ReadWord(node + 8u), context);
        node = runtime.ReadWord(node);
    }
}

template <class Draw>
void DrawTree(std::uint32_t system, std::uint32_t tree_offset,
              std::uint32_t object_offset, std::uint32_t context,
              IEffectSystemGroupDrawRuntime& runtime, Draw&& draw) {
    if (runtime.ReadByte(system + 4u) == 0u) return;
    const auto tree = system + tree_offset;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        draw(runtime.ReadWord(node + object_offset), context);
        node = MonsterManagerTreeCursor::Successor(
            node, runtime.SharedNil(tree),
            [&](std::uint32_t address) {
                return runtime.ReadWord(address);
            }).next;
    }
}

}  // namespace

void EffectSystem::DrawActivePlaceholder(
    std::uint32_t system, std::uint32_t context,
    IEffectSystemGroupDrawRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return;

    const auto tree = system + 12u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        runtime.NoOpDraw(context);
        node = MonsterManagerTreeCursor::Successor(
            node, runtime.SharedNil(tree),
            [&](std::uint32_t address) {
                return runtime.ReadWord(address);
            }).next;
    }
}

void EffectSystem::DrawObjectGroups(
    std::uint32_t system, std::uint32_t context,
    IEffectSystemGroupDrawRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return;

    constexpr std::uint32_t before_state[] = {
        164u, 68u, 92u, 116u, 128u, 152u, 104u, 140u};
    constexpr std::uint32_t after_state[] = {
        80u, 176u, 188u, 200u};
    for (const auto offset : before_state)
        DrawList(system, offset, context, runtime);
    SetRenderStates(system, runtime);
    for (const auto offset : after_state)
        DrawList(system, offset, context, runtime);
}

void EffectSystem::DrawXmlEffects(
    std::uint32_t system, std::uint32_t context,
    IEffectSystemGroupDrawRuntime& runtime) {
    DrawTree(system, 228u, 16u, context, runtime,
             [&](std::uint32_t object, std::uint32_t draw_context) {
                 runtime.DrawXmlObject(object, draw_context);
             });
}

void EffectSystem::DrawInlineEffects(
    std::uint32_t system, std::uint32_t context,
    IEffectSystemGroupDrawRuntime& runtime) {
    DrawTree(system, 212u, 12u, context, runtime,
             [&](std::uint32_t object, std::uint32_t draw_context) {
                 runtime.DrawObject(object, draw_context);
             });
}

}  // namespace pal4
