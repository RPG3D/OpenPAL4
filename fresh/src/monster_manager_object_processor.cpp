#include "pal4/monster_manager_object_processor.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {

std::uint32_t MonsterManagerObjectProcessRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint8_t MonsterManagerObjectProcessRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void MonsterManagerObjectProcessRuntimeStub::UpdateGrid(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerObjectProcessRuntimeStub::CallSlot34(
    std::uint32_t, std::uint32_t) {}
void MonsterManagerObjectProcessRuntimeStub::CallSlot38(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void MonsterManagerObjectProcessRuntimeStub::ProcessCollision(
    std::uint32_t, float, std::uint32_t) {}
void MonsterManagerObjectProcessRuntimeStub::SetPursuitTarget(
    std::uint32_t, std::uint32_t) {}

std::uint32_t MonsterManagerObjectProcessor::Process(
    std::uint32_t manager, std::uint32_t argument,
    MonsterManagerTreeGlobals& globals,
    IMonsterManagerObjectProcessRuntime& runtime) {
    return Walk(manager, argument, 0u, 0.0f, Mode::Basic,
                globals, runtime);
}

std::uint32_t MonsterManagerObjectProcessor::ProcessExtended(
    std::uint32_t manager, std::uint32_t first,
    std::uint32_t second, MonsterManagerTreeGlobals& globals,
    IMonsterManagerObjectProcessRuntime& runtime) {
    if (runtime.ReadByte(manager + 0x5cu) != 0u) {
        runtime.UpdateGrid(manager + 0x38u, first);
    }
    return Walk(manager, first, second, 0.0f, Mode::Extended,
                globals, runtime);
}

std::uint32_t MonsterManagerObjectProcessor::ProcessCollisions(
    std::uint32_t manager, float incoming_st0,
    std::uint32_t argument, MonsterManagerTreeGlobals& globals,
    IMonsterManagerObjectProcessRuntime& runtime) {
    return Walk(manager, argument, 0u, incoming_st0, Mode::Collision,
                globals, runtime);
}

std::uint32_t MonsterManagerObjectProcessor::SetPursuitTargetAll(
    std::uint32_t manager, std::uint32_t target,
    MonsterManagerTreeGlobals& globals,
    IMonsterManagerObjectProcessRuntime& runtime) {
    return Walk(manager, target, 0u, 0.0f, Mode::PursuitTarget,
                globals, runtime);
}

std::uint32_t MonsterManagerObjectProcessor::Walk(
    std::uint32_t manager, std::uint32_t first,
    std::uint32_t second, float incoming_st0, Mode mode,
    MonsterManagerTreeGlobals& globals,
    IMonsterManagerObjectProcessRuntime& runtime) {
    const auto header = runtime.ReadDword(manager + 0x2cu);
    auto node = runtime.ReadDword(header);
    if (node == header) return header;

    auto nil = globals.nil_address;
    std::uint32_t result = header;
    do {
        const auto object = runtime.ReadDword(node + 0x1cu);
        if (runtime.ReadDword(object + 0x1f0u) != 0u) {
            bool invoked = false;
            switch (mode) {
            case Mode::Basic:
                runtime.CallSlot34(object, first);
                invoked = true;
                break;
            case Mode::Extended:
                runtime.CallSlot38(object, first, second);
                invoked = true;
                break;
            case Mode::Collision:
                if (runtime.ReadDword(object + 0x730u) == 1u) break;
                runtime.ProcessCollision(object, incoming_st0, first);
                invoked = true;
                break;
            case Mode::PursuitTarget:
                runtime.SetPursuitTarget(object, first);
                invoked = true;
                break;
            }
            if (invoked) nil = globals.nil_address;
        }
        const auto step = MonsterManagerTreeCursor::Successor(
            node, nil,
            [&runtime](std::uint32_t address) {
                return runtime.ReadDword(address);
            });
        node = step.next;
        result = step.result;
    } while (node != runtime.ReadDword(manager + 0x2cu));
    return result;
}

}  // namespace pal4
