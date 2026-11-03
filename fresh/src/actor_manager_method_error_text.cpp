#include "pal4/actor_manager_method_error_text.h"

namespace pal4 {
namespace {

struct ErrorTextConfig {
    std::uint32_t guard;
    std::uint32_t record;
    std::uint32_t cleanup;
};

std::uint32_t GetOrHandle(const std::uint32_t object,
                          const std::uint32_t argument,
                          const ErrorTextConfig config,
                          IActorManagerMethodErrorRuntime& runtime) {
    const auto guard = runtime.ReadByte(config.guard);
    if ((guard & 1u) == 0u) {
        runtime.WriteByte(config.guard,
                          static_cast<std::uint8_t>(guard | 1u));
        runtime.WriteByte(config.record,
                          static_cast<std::uint8_t>(argument));
        runtime.WriteWord(config.record + 4u, 0u);
        runtime.WriteWord(config.record + 8u, 0u);
        runtime.WriteWord(config.record + 12u, 0u);
        if (runtime.AllocateText(
                config.record,
                static_cast<std::uint32_t>(ActorManagerMethodErrorText::
                                               kMessage.size()),
                1u)) {
            const auto text = runtime.ReadWord(config.record + 4u);
            runtime.CopyText(text, ActorManagerMethodErrorText::kMessage);
            runtime.WriteWord(
                config.record + 8u,
                static_cast<std::uint32_t>(
                    ActorManagerMethodErrorText::kMessage.size()));
            runtime.WriteByte(
                text + static_cast<std::uint32_t>(
                           ActorManagerMethodErrorText::kMessage.size()),
                0u);
        }
        runtime.RegisterCleanup(config.cleanup);
    }
    const auto handler = runtime.ReadWord(object + 8u);
    return handler ? runtime.InvokeHandler(handler, argument) : config.record;
}

void Cleanup(const ErrorTextConfig config,
             IActorManagerMethodErrorRuntime& runtime) {
    const auto text = runtime.ReadWord(config.record + 4u);
    if (text) {
        const auto header = text - 1u;
        const auto references = runtime.ReadByte(header);
        if (references == 0u || references == 0xffu)
            runtime.Free(header);
        else
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1u));
    }
    runtime.WriteWord(config.record + 4u, 0u);
    runtime.WriteWord(config.record + 8u, 0u);
    runtime.WriteWord(config.record + 12u, 0u);
}

}  // namespace

std::uint32_t ActorManagerMethodErrorText::GetOrHandle(
    const std::uint32_t object, const std::uint32_t argument,
    IActorManagerMethodErrorRuntime& runtime) {
    return pal4::GetOrHandle(object, argument,
                            {kGuard, kRecord, kCleanup}, runtime);
}

void ActorManagerMethodErrorText::Cleanup(
    IActorManagerMethodErrorRuntime& runtime) {
    pal4::Cleanup({kGuard, kRecord, kCleanup}, runtime);
}

std::uint32_t ActorManagerPlayerMethodErrorText::GetOrHandle(
    const std::uint32_t object, const std::uint32_t argument,
    IActorManagerMethodErrorRuntime& runtime) {
    return pal4::GetOrHandle(object, argument,
                            {kGuard, kRecord, kCleanup}, runtime);
}

void ActorManagerPlayerMethodErrorText::Cleanup(
    IActorManagerMethodErrorRuntime& runtime) {
    pal4::Cleanup({kGuard, kRecord, kCleanup}, runtime);
}

}  // namespace pal4
