#include "pal4/gob_sound.h"
#include "pal4/gob_sound_tick.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Near(float left, float right) {
    return std::fabs(left - right) < 0.0001f;
}

struct Runtime final : pal4::IGobSoundTickRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::uint32_t config = 0x8000u;
    std::uint32_t play_result = 0x9000u;
    float distance = 5.0f;
    float radius = 10.0f;
    float next_delay = 2.5f;
    bool playing_initial = false;
    bool playing_after_delay = false;
    unsigned play_checks = 0;
    std::string sound_name = "Wind";

    Runtime() {
        floats[object + 0xacu] = 1.0f;
        floats[object + 0xb0u] = 2.0f;
        floats[object + 0xb4u] = 3.0f;
        floats[object + 0x1f4u] = 1.0f;
        floats[object + 0x1f8u] = 3.0f;
        floats[object + 0x1fcu] = 0.25f;
        words[object + 0x1e8u] = 0x7000u;
        words[object + 0x198u] = 4u;
    }
    std::uint32_t GetSoundConfigNode(std::uint32_t address) override {
        Check(address == object, "config object");
        calls.emplace_back("get config");
        return config;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word " + std::to_string(address - object));
    }
    float ReadFloat(std::uint32_t address) override { return floats[address]; }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write float " + std::to_string(address - object));
    }
    std::string_view ReadCString(std::uint32_t address) override {
        Check(address == 0x7000u, "sound name pointer");
        calls.emplace_back("read name");
        return sound_name;
    }
    bool IsAudioPlaying(std::uint32_t handle) override {
        Check(handle == object + 0x1e0u, "audio handle");
        calls.emplace_back("is playing");
        return play_checks++ == 0 ? playing_initial :
                                    playing_after_delay;
    }
    pal4::GobSoundVector ActiveCameraPosition() override {
        calls.emplace_back("camera position");
        return {4.0f, 5.0f, 6.0f};
    }
    float CalculateVectorMagnitude(
        const pal4::GobSoundVector& vector) override {
        Check(Near(vector.x, 3.0f) && Near(vector.y, 0.0f) &&
              Near(vector.z, 3.0f), "camera relative vector");
        calls.emplace_back("magnitude");
        return distance;
    }
    float AudibleRadius(std::uint32_t address) override {
        Check(address == object, "audible radius object");
        calls.emplace_back("audible radius");
        return radius;
    }
    std::uint32_t TimeMilliseconds() override {
        calls.emplace_back("time");
        return 1234u;
    }
    float RandomDelay(float minimum, float maximum, float seed) override {
        Check(Near(minimum, 1.0f) && Near(maximum, 3.0f) &&
              Near(seed, 1234.0f), "random delay arguments");
        calls.emplace_back("random delay");
        return next_delay;
    }
    std::string BuildSoundPath(std::string_view name) override {
        Check(name == "Wind", "build path name");
        calls.emplace_back("build path");
        return "PALSound/Wind.wav";
    }
    std::uint32_t Play3DSound(
        const pal4::GobSoundVector& source, std::string_view path,
        const pal4::GobSoundVector& emitter, std::uint32_t use_count,
        float playback_radius, float near_radius) override {
        const auto expected_source = words[object + 0x198u] ==
                0xffffffffu ? pal4::GobSoundVector{4.0f, 5.0f, 6.0f} :
                               pal4::GobSoundVector{3.0f, 0.0f, 3.0f};
        Check(Near(source.x, expected_source.x) &&
              Near(source.y, expected_source.y) &&
              Near(source.z, expected_source.z) &&
              path == "PALSound/Wind.wav" &&
              Near(emitter.x, 1.0f) && Near(emitter.y, 2.0f) &&
              Near(emitter.z, 3.0f) &&
              use_count == (words[object + 0x198u] == 0xffffffffu ?
                            0u : 4u) &&
              Near(playback_radius, 10.0f) &&
              Near(near_radius, 2.5f), "3D sound arguments");
        calls.emplace_back("play 3D");
        return play_result;
    }
    void ApplyMovement(std::uint32_t address,
                       const pal4::GobSoundVector& movement,
                       std::uint32_t flag) override {
        Check(address == object && flag == 1u &&
              Near(movement.x, 0.0f) &&
              Near(movement.y, 100.0f) &&
              Near(movement.z, 0.0f), "sound movement args");
        calls.emplace_back("apply movement");
    }
    std::uint32_t UpdateClumpIfVisible(std::uint32_t address,
                                        float elapsed) override {
        Check(address == object && Near(elapsed, 0.5f),
              "visible clump args");
        calls.emplace_back("update clump");
        return 0x99u;
    }
};

void CheckGates() {
    Runtime no_config;
    no_config.config = 0u;
    Check(pal4::GobSound::TickAndMaybePlay3D(
              Runtime::object, 0.5f, no_config) == 0u &&
          no_config.calls == (std::vector<std::string>{"get config"}),
          "missing config bypasses tick");
    Runtime suppressed;
    suppressed.words[Runtime::object + 0x1dcu] = 1u;
    Check(pal4::GobSound::TickAndMaybePlay3D(
              Runtime::object, 0.5f, suppressed) == 0x8000u &&
          suppressed.calls == (std::vector<std::string>{"get config"}),
          "suppressed playback bypasses movement");
    Runtime distant;
    distant.distance = 20.0f;
    Check(pal4::GobSound::TickAndMaybePlay3D(
              Runtime::object, 0.5f, distant) == 0x99u &&
          distant.words[Runtime::object + 0x204u] == 1u &&
          distant.floats[Runtime::object + 0x200u] == 0.0f &&
          distant.calls[distant.calls.size() - 2] == "apply movement" &&
          distant.calls.back() == "update clump",
          "distant source skips timer but still updates movement");
    Runtime already_playing;
    already_playing.playing_initial = true;
    Check(pal4::GobSound::TickAndMaybePlay3D(
              Runtime::object, 0.5f, already_playing) == 0x99u &&
          already_playing.floats[Runtime::object + 0x200u] == 0.0f &&
          std::find(already_playing.calls.begin(),
                    already_playing.calls.end(), "play 3D") ==
              already_playing.calls.end(),
          "already playing sound does not set pending timer");
}

void CheckTimerAndPlayback() {
    Runtime waiting;
    waiting.floats[Runtime::object + 0x1fcu] = 1.0f;
    Check(pal4::GobSound::TickAndMaybePlay3D(
              Runtime::object, 0.5f, waiting) == 0x99u &&
          Near(waiting.floats[Runtime::object + 0x200u], 0.5f) &&
          waiting.words[Runtime::object + 0x204u] == 1u &&
          std::find(waiting.calls.begin(), waiting.calls.end(),
                    "random delay") == waiting.calls.end(),
          "pending sound accumulates elapsed time");
    Runtime played;
    Check(pal4::GobSound::TickAndMaybePlay3D(
              Runtime::object, 0.5f, played) == 0x99u &&
          Near(played.floats[Runtime::object + 0x200u], 0.0f) &&
          Near(played.floats[Runtime::object + 0x1fcu], 2.5f) &&
          played.words[Runtime::object + 0x204u] == 0u &&
          played.words[Runtime::object + 0x1e0u] == 0x9000u &&
          played.calls == (std::vector<std::string>{
              "get config", "is playing", "write word 516",
              "camera position", "magnitude", "magnitude", "audible radius",
              "write float 512", "write float 512", "time", "random delay",
              "write float 508", "write word 516", "is playing",
              "read name", "build path", "audible radius",
              "audible radius", "play 3D", "write word 480",
              "apply movement", "update clump"}),
          "timer expiry reseeds and starts 3D playback in order");
    Runtime negative;
    negative.words[Runtime::object + 0x198u] = 0xffffffffu;
    Check(pal4::GobSound::TickAndMaybePlay3D(
              Runtime::object, 0.5f, negative) == 0x99u &&
          negative.words[Runtime::object + 0x1e0u] == 0x9000u,
          "negative use count plays from active camera with zero count");
    Runtime empty;
    empty.sound_name.clear();
    Check(pal4::GobSound::TickAndMaybePlay3D(
              Runtime::object, 0.5f, empty) == 0x99u &&
          empty.words[Runtime::object + 0x204u] == 0u &&
          std::find(empty.calls.begin(), empty.calls.end(),
                    "play 3D") == empty.calls.end(),
          "empty name resets timer without playback");
    Runtime resumed;
    resumed.playing_after_delay = true;
    Check(pal4::GobSound::TickAndMaybePlay3D(
              Runtime::object, 0.5f, resumed) == 0x99u &&
          std::find(resumed.calls.begin(), resumed.calls.end(),
                    "play 3D") == resumed.calls.end(),
          "playback started before second check prevents duplicate sound");
}

}  // namespace

int main() {
    try {
        CheckGates();
        CheckTimerAndPlayback();
        std::cout << "gob_sound_tick_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
