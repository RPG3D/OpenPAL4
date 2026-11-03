#include "pal4/combat_stunt_sequence_task.h"

#include "pal4/check_protect_task.h"
#include "pal4/start_attack_task.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 3> kTiming{1u, 0u, 0u};

std::uint32_t TextOrEmpty(IEndStuntStartRuntime& runtime,
                          const std::uint32_t global) {
    const auto text = runtime.ReadWord(global);
    return text == 0u ? 0x841964u : text;
}

struct RunResult {
    bool created;
    std::uint32_t value;
};

RunResult AddRun(const std::uint32_t task,
                 const std::uint32_t factory_vtable,
                 const std::uint32_t text_global,
                 IEndStuntStartRuntime& runtime) {
    const auto text = TextOrEmpty(runtime, text_global);
    const auto child = runtime.CreateChildTask(factory_vtable);
    if (child == 0u) return {false, 0u};
    runtime.AddTaskAndInitialize(task + 4u, child, text, kTiming);
    const auto source = runtime.ReadWord(task + 44u);
    const auto position = runtime.ActorPosition(source);
    return {true, runtime.SetApproachTaskTarget(
        child, runtime.ReadWord(task + 44u), position)};
}

}  // namespace

std::uint32_t EndStuntSequenceTask::Start(
    const std::uint32_t task, IEndStuntStartRuntime& runtime) {
    const auto stunt = runtime.ReadWord(task + 52u);
    const auto mode = runtime.ReadWord(stunt + 132u);
    if (mode != 1u && mode != 2u) return mode;

    const auto source = runtime.ReadWord(task + 44u);
    const auto target = runtime.ReadWord(task + 48u);
    const auto source_side = runtime.ReadWord(source + 804u);
    if (source_side == runtime.ReadWord(target + 804u))
        return source_side;

    if (mode == 1u) {
        const auto first = AddRun(task,
                                  StartAttackApproachFactory::kPrimaryVtable,
                                  0x8f9784u, runtime);
        if (!first.created) return 0u;
        return AddRun(task, StartAttackApproachFactory::kSecondaryVtable,
                      0x8f97acu, runtime).value;
    }

    const auto text = TextOrEmpty(runtime, 0x8f9798u);
    CheckProtectSubtaskFactorySlot factory;
    (void)CheckProtectSubtaskFactory::InitializePosition(&factory);
    const auto child = runtime.CreateChildTask(factory.vtable);
    if (child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, child, text, kTiming);
    const auto current_source = runtime.ReadWord(task + 44u);
    const auto position = runtime.ActorPosition(current_source);
    runtime.WriteWord(child + 72u, runtime.ReadWord(task + 44u));
    for (std::size_t i = 0u; i < position.values.size(); ++i)
        runtime.WriteWord(child + 60u +
                              static_cast<std::uint32_t>(i) * 4u,
                          std::bit_cast<std::uint32_t>(position.values[i]));
    return position.address;
}

}  // namespace pal4
