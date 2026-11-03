#pragma once

#include "pal4/script_party_recovery.h"
#include "pal4/script_player_rest.h"
#include <bit>
#include <functional>
#include <utility>

namespace pal4 {

// Borrows the existing stat owner and a direct sample-play boundary. Rest sounds
// do not belong to the script sound-ID registry or its current/waiting handle.
class NativeScriptPlayerRest final : public IScriptPlayerRestRuntime {
public:
    using AudioManagerGetter = std::function<std::uint32_t()>;
    using SamplePlayer = std::function<std::uint32_t(
        std::uint32_t, std::string_view, std::uint32_t)>;
    NativeScriptPlayerRest(IScriptPartyRecoveryRuntime& players,
        AudioManagerGetter audio_manager, SamplePlayer play_sample)
        : players_(players), audio_manager_(std::move(audio_manager)),
          play_sample_(std::move(play_sample)) {}
    std::uint32_t PlayerManager() override { return players_.PlayerManager(); }
    std::uint32_t FindPlayer(std::uint32_t manager, std::uint32_t index) override {
        return players_.PlayerByIndex(manager, index);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return players_.ReadWord(address);
    }
    std::int32_t GetStat(std::uint32_t bank, std::uint32_t slot) override {
        return std::bit_cast<std::int32_t>(players_.ReadMaxStat(bank, slot));
    }
    void SetStat(std::uint32_t bank, std::uint32_t slot, std::int32_t value) override {
        (void)players_.WriteCurrentStat(bank, slot, std::bit_cast<std::uint32_t>(value));
    }
    std::uint32_t AudioManager() override { return audio_manager_(); }
    std::uint32_t PlayMusic(std::uint32_t manager, std::string_view path,
        std::uint32_t mode) override { return play_sample_(manager, path, mode); }
private:
    IScriptPartyRecoveryRuntime& players_;
    AudioManagerGetter audio_manager_;
    SamplePlayer play_sample_;
};

} // namespace pal4
