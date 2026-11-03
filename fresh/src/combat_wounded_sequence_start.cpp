#include "pal4/combat_wounded_sequence_start.h"

#include "pal4/combat_wounded_sequence_task.h"
#include "pal4/combat_miss_sequence_task.h"
#include "pal4/win_action_task.h"

namespace pal4 {
namespace {

struct SoundTextGuard {
    IWoundedSequenceStartRuntime& runtime;
    std::uint32_t temporary_text;
    bool active = true;

    ~SoundTextGuard() {
        if (active) runtime.ReleaseSoundText(temporary_text, 1u);
    }
};

std::uint32_t TextOrEmpty(IWoundedSequenceStartRuntime& runtime,
                          std::uint32_t address) {
    const auto value = runtime.ReadWord(address);
    return value == 0u ? 0x841964u : value;
}

}  // namespace

std::uint32_t WoundedSequenceTask::Start(
    std::uint32_t task, IWoundedSequenceStartRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    const auto actor = runtime.ReadWord(task + 44u);
    const auto state = runtime.ActorVirtual168(actor);
    runtime.CheckActorStateGroup(runtime.ReadWord(task + 44u));
    if (state == 3u)
        runtime.SetActorState(runtime.ReadWord(task + 44u), 3u, 1u);

    const auto sound = runtime.ActorVirtual456(
        runtime.ReadWord(task + 44u), 0u);
    SoundTextGuard guard{runtime, sound.temporary_text};
    runtime.PlaySoundEffect(sound.value, 1u);
    guard.active = false;
    runtime.ReleaseSoundText(sound.temporary_text, 1u);

    const auto message_text = TextOrEmpty(runtime, 0x8f960cu);
    MissMessageChildFactorySlot message_factory;
    (void)MissMessageChildFactory::Initialize(&message_factory);
    const auto message = runtime.CreateChildTask(message_factory.vtable);
    if (message == 0u) return 0u;
    (void)runtime.AddTaskAndInitialize(task + 4u, message,
                                        message_text, timing);
    runtime.WriteWord(message + 60u, runtime.ReadWord(task + 44u));
    runtime.AssignText(message + 64u, 0x8fa31cu, 0u, 0xffffffffu);

    const auto finish_text = TextOrEmpty(runtime, 0x8f9770u);
    WinActionChildFactorySlot finish_factory;
    (void)WinActionSecondChildFactory::Initialize(&finish_factory);
    const auto finish = runtime.CreateChildTask(finish_factory.vtable);
    if (finish == 0u) return 0u;
    const auto result = runtime.AddTaskAndInitialize(
        task + 4u, finish, finish_text, timing);
    runtime.WriteWord(finish + 60u, runtime.ReadWord(task + 44u));
    return result;
}

}  // namespace pal4
