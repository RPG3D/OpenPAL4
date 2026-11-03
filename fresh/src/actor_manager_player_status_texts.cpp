#include "pal4/actor_manager_player_status_texts.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

namespace {
constexpr std::array<std::string_view, 4> kTexts = {
    "101", "103", "106", "105"};
constexpr std::uint32_t kRecordSize = 16u;
}  // namespace

std::string_view ActorManagerPlayerStatusTexts::TextForRole(std::uint32_t index) {
    return kTexts[index<kTexts.size()?index:0u];
}

std::uint32_t ActorManagerPlayerStatusTexts::Lookup(
    const std::uint32_t actor,
    IActorManagerPlayerStatusTextRuntime& runtime) {
    if (!initialized_) {
        initialized_ = true;

        // The binary copies an uninitialized stack byte into each flag.
        // Zero is the defined starting value for the reconstructed records.
        runtime.WriteByte(kRecords, 0u);
        for (const auto offset : {4u, 8u, 12u})
            runtime.WriteWord(kRecords + offset, 0u);
        runtime.SetExceptionMessage(kRecords, kTexts[0]);

        for (std::uint32_t i = 1u; i < kTexts.size(); ++i) {
            const auto record = kRecords + i * kRecordSize;
            runtime.WriteByte(record, 0u);
            runtime.SetErrorFlag(record, 0u);
            const auto text = kTexts[i];
            if (runtime.AllocateText(record,
                                     static_cast<std::uint32_t>(text.size()),
                                     1u)) {
                const auto buffer = runtime.ReadWord(record + 4u);
                runtime.CopyText(buffer, text);
                runtime.WriteWord(record + 8u,
                                  static_cast<std::uint32_t>(text.size()));
                runtime.WriteByte(buffer +
                                      static_cast<std::uint32_t>(text.size()),
                                  0u);
            }
        }
        runtime.RegisterExit(kCleanupEntry);
    }

    auto index = runtime.ReadSignedWord(actor + 1192u);
    if (index >= 4) index = 0;
    const auto record = kRecords + static_cast<std::uint32_t>(index) *
                                      kRecordSize;
    const auto text = runtime.ReadWord(record + 4u);
    return text ? text : kEmptyText;
}

void ActorManagerPlayerStatusTexts::Cleanup(
    IActorManagerPlayerStatusTextRuntime& runtime) const {
    for (std::uint32_t i = 4u; i != 0u; --i)
        runtime.ReleaseText(kRecords + (i - 1u) * kRecordSize);
}

}  // namespace pal4
