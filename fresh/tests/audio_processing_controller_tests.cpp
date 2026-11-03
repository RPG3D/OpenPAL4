#include "pal4/audio_processing_controller.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string_view operation;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IAudioProcessingRuntime {
    std::vector<Call> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t manager = 0x8123u;
    bool clear_list_on_set = false;

    Runtime() {
        words[manager] = 0x3f800000u;
        words[manager + 36u] = 0x9000u;
        words[manager + 40u] = 0x9008u;
        words[0x9000u] = 0xa000u;
        words[0x9004u] = 0u;
    }

    std::uint32_t AudioManagerInstance() override {
        calls.push_back({"manager"});
        return manager;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back({"read", address});
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back({"write", address, value});
        words[address] = value;
    }
    std::uint32_t SetStreamVolume(const std::uint32_t stream,
                                  const std::uint32_t bits) override {
        calls.push_back({"stream-volume", stream, bits});
        if (clear_list_on_set) words[manager + 36u] = 0u;
        return 0x66u;
    }
};

void EnableAndRestoreVolume() {
    pal4::AudioProcessingState state{};
    pal4::AudioProcessingController controller(state);
    Runtime enable;
    Check(controller.SetProcessingState(1u, enable) == 2u &&
              state.enabled == 1u &&
              state.saved_volume_bits == 0x3f800000u &&
              enable.words.at(0x8123u) == 0u,
          "enable stores original float bits and mutes manager");
    Check(enable.calls == std::vector<Call>{
              {"manager"}, {"read", 0x8123u},
              {"manager"}, {"write", 0x8123u},
              {"read", 0x8147u}, {"read", 0x814bu},
              {"read", 0x9000u}, {"read", 0x8123u},
              {"stream-volume", 0xa000u},
              {"read", 0x8147u}, {"read", 0x814bu},
              {"read", 0x9004u},
              {"read", 0x8147u}, {"read", 0x814bu}},
          "enable scans both stream slots and skips the null stream");

    Runtime restore;
    restore.words[restore.manager] = 0u;
    Check(controller.SetProcessingState(0u, restore) == 2u &&
              state.enabled == 0u &&
              restore.words.at(restore.manager) == 0x3f800000u &&
              restore.calls.front() == Call{"manager"} &&
              restore.calls[1] ==
                  Call{"write", restore.manager, 0x3f800000u} &&
              restore.calls[6] ==
                  Call{"stream-volume", 0xa000u, 0x3f800000u},
          "disable restores saved volume before applying stream volume");
}

void UnchangedAndEmptyListReturns() {
    pal4::AudioProcessingState state{0x3f400000u, 2u};
    pal4::AudioProcessingController controller(state);
    Runtime unchanged;
    Check(controller.SetProcessingState(2u, unchanged) == 2u &&
              unchanged.calls.empty(),
          "unchanged processing state performs no runtime calls");

    Runtime no_streams;
    no_streams.words[no_streams.manager + 36u] = 0u;
    state.enabled = 0u;
    Check(controller.SetProcessingState(1u, no_streams) == 0x23u &&
              state.enabled == 1u && no_streams.calls.size() == 5u,
          "empty stream list preserves low byte of manager result");

    Runtime changed_during_callback;
    changed_during_callback.clear_list_on_set = true;
    state.enabled = 0u;
    Check(controller.SetProcessingState(
              1u, changed_during_callback) == 0x66u,
          "stream removal during callback preserves last setter return byte");
}

}  // namespace

int main() {
    try {
        EnableAndRestoreVolume();
        UnchangedAndEmptyListReturns();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
