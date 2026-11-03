#include "pal4/actor_manager_player_capture_chance.h"

#include <bit>

namespace pal4 {

bool ActorManagerPlayerCaptureChance::Check(
    const std::uint32_t actor, const std::uint32_t target,
    IActorManagerPlayerCaptureRuntime& runtime) {
    if (runtime.ReadSignedWord(target + 804u) != 1) return false;
    const auto state = runtime.ActorSlot144(actor);
    if (state == 1 || (state > 3 && state <= 6) || target == actor)
        return false;
    const auto kind = runtime.TargetSlot168(target);
    if (kind != 1 && kind != 2) return false;

    auto level = runtime.ReadSignedWord(target + 1192u);
    if (level >= 0 && level < 4)
        level = runtime.ReadSignedWord(
            target + 1176u + static_cast<std::uint32_t>(level) * 4u);
    const auto level_float = static_cast<float>(level);
    const auto divisor = runtime.FloatElement(runtime.Database() + 184u, 12u);
    const auto ratio = static_cast<float>(
        static_cast<double>(level_float) / static_cast<double>(divisor));
    const auto weight = runtime.FloatElement(runtime.Database() + 184u, 13u);
    const auto weighted = static_cast<float>(
        static_cast<double>(weight) * static_cast<double>(ratio));
    const auto base = runtime.FloatElement(runtime.Database() + 184u, 14u);
    constexpr auto kKindOneBonus = std::bit_cast<float>(0x3d4ccccdu);
    constexpr auto kKindTwoBonus = std::bit_cast<float>(0x3dcccccdu);
    const auto threshold = static_cast<float>(
        static_cast<double>(base) + static_cast<double>(weighted) +
        static_cast<double>(kind == 1 ? kKindOneBonus : kKindTwoBonus));
    constexpr auto kThousandth = std::bit_cast<float>(0x3a83126fu);
    const auto random = runtime.RandomNumber() % 1000u;
    return static_cast<double>(random) *
               static_cast<double>(kThousandth) < threshold;
}

}  // namespace pal4
