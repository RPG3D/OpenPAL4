#include "pal4/global_data_settings.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGlobalDataSettingsRuntime {
    std::vector<std::uint8_t> bytes;
    std::vector<std::size_t> write_sizes;
    std::vector<std::size_t> read_sizes;
    std::vector<std::string> calls;
    std::array<float, 3> levels{};
    std::array<std::vector<std::uint32_t>, 3> streams{};
    std::size_t cursor = 0;
    std::uint32_t manager_calls = 0;
    std::int32_t ui_result = -9;
    bool has_effect_system = true;
    float atomic_value = 0.0f;
    float effect_value = 0.0f;

    void WriteBytes(std::uint32_t archive, const void* data,
                    std::size_t length) override {
        Check(archive == 0x1234, "save archive handle");
        write_sizes.push_back(length);
        const auto* first = static_cast<const std::uint8_t*>(data);
        bytes.insert(bytes.end(), first, first + length);
        calls.emplace_back("write");
    }
    void ReadBytes(std::uint32_t archive, void* data,
                   std::size_t length) override {
        Check(archive == 0x1234 && cursor + length <= bytes.size(),
              "load archive bounds");
        read_sizes.push_back(length);
        std::memcpy(data, bytes.data() + cursor, length);
        cursor += length;
        calls.emplace_back("read");
    }
    std::uint32_t AudioManager() override {
        ++manager_calls;
        calls.emplace_back("manager");
        return 0x4321;
    }
    float AudioLevel(std::uint32_t manager, std::uint32_t index) override {
        Check(manager == 0x4321 && index < 2, "saved audio level");
        calls.emplace_back("get:" + std::to_string(index));
        return levels[index];
    }
    void SetAudioLevel(std::uint32_t manager, std::uint32_t index,
                       float value) override {
        Check(manager == 0x4321 && index < 3, "loaded audio level");
        levels[index] = value;
        calls.emplace_back("set:" + std::to_string(index));
    }
    std::size_t AudioStreamCount(
        std::uint32_t manager,
        pal4::GlobalDataAudioCollection collection) override {
        Check(manager == 0x4321, "stream collection manager");
        return streams[static_cast<std::size_t>(collection)].size();
    }
    std::uint32_t AudioStreamAt(
        std::uint32_t manager, pal4::GlobalDataAudioCollection collection,
        std::size_t index) override {
        Check(manager == 0x4321, "stream manager");
        return streams[static_cast<std::size_t>(collection)].at(index);
    }
    std::uint32_t AudioStreamKind(std::uint32_t stream) override {
        calls.emplace_back("kind:" + std::to_string(stream));
        return stream % 10;
    }
    void SetStreamVolume(std::uint32_t stream, float value) override {
        Check(std::bit_cast<std::uint32_t>(value) ==
                  std::bit_cast<std::uint32_t>(levels[0]),
              "first collection receives first level");
        calls.emplace_back("volume:" + std::to_string(stream));
    }
    void RefreshStream(std::uint32_t stream) override {
        calls.emplace_back("refresh:" + std::to_string(stream));
    }
    void SetAtomicPluginFloatData(float value) override {
        atomic_value = value;
        calls.emplace_back("atomic");
    }
    bool HasEffectSystem() override {
        calls.emplace_back("available");
        return has_effect_system;
    }
    std::int32_t SetEffectSystemSquaredValue(float value) override {
        effect_value = value;
        calls.emplace_back("effect");
        return ui_result;
    }
};

std::vector<std::uint8_t> ExpectedBytes(
    const std::array<std::uint32_t, 5>& words, std::uint8_t tail,
    std::uint32_t first, std::uint32_t second) {
    std::vector<std::uint8_t> result;
    auto append = [&result](std::uint32_t word) {
        for (unsigned shift = 0; shift < 32; shift += 8)
            result.push_back(static_cast<std::uint8_t>(word >> shift));
    };
    for (const auto word : words) append(word);
    result.push_back(tail);
    append(first);
    append(second);
    return result;
}

void TestSaveAndLoad() {
    constexpr std::array<std::uint32_t, 5> words{
        0x12345678, 0x90abcdef, 3, 0, 5};
    pal4::GlobalDataSettingsState state{words, 0x7f};
    Runtime runtime;
    runtime.levels = {std::bit_cast<float>(0x3e800000u),
                      std::bit_cast<float>(0x3f200000u), 0.0f};
    Check(pal4::GlobalDataSettings::Save(state, 0x1234, runtime) == 1,
          "save return");
    Check(runtime.bytes == ExpectedBytes(words, 0x7f, 0x3e800000,
                                         0x3f200000),
          "29-byte little-endian settings layout");
    Check(runtime.write_sizes == std::vector<std::size_t>{4, 4, 4, 4, 4,
                                                          1, 4, 4},
          "save field boundaries");
    Check(runtime.manager_calls == 2 &&
              runtime.calls == std::vector<std::string>{
                  "write", "write", "write", "write", "write", "write",
                  "manager", "get:0", "write", "manager", "get:1",
                  "write"},
          "save reads audio manager separately for each level");

    state = {};
    runtime.calls.clear();
    runtime.manager_calls = 0;
    runtime.streams = {{{0, 10, 11}, {0, 21, 22}, {0, 31, 32}}};
    Check(pal4::GlobalDataSettings::Load(state, 0x1234, runtime) == 1,
          "load returns one regardless of UI result");
    Check(state.words == words && state.byte_14 == 0x7f &&
              runtime.cursor == 29,
          "load restores every setting byte");
    Check(runtime.read_sizes == runtime.write_sizes,
          "load field boundaries match save");
    Check(runtime.manager_calls == 3 && runtime.atomic_value == 1800.0f &&
              runtime.effect_value == 1800.0f,
          "load obtains manager three times and applies zero scale option");
    Check(std::bit_cast<std::uint32_t>(runtime.levels[0]) == 0x3e800000 &&
              std::bit_cast<std::uint32_t>(runtime.levels[1]) == 0x3f200000 &&
              std::bit_cast<std::uint32_t>(runtime.levels[2]) == 0x3f200000,
          "second saved level drives both effect collections");
    Check(runtime.calls == std::vector<std::string>{
              "read", "read", "read", "read", "read", "read", "read",
              "manager", "set:0", "volume:10", "volume:11", "read",
              "manager", "set:1", "kind:21", "refresh:21", "kind:22",
              "manager", "set:2", "kind:31", "kind:32", "refresh:32",
              "atomic", "available", "effect"},
          "load audio stream filtering and operation order");
}

void TestDefaultsAndEmptyCollections() {
    Runtime runtime;
    pal4::GlobalDataSettingsState state{};
    runtime.ui_result = 73;
    Check(pal4::GlobalDataSettings::ResetDefaults(state, runtime) == 73,
          "defaults return UI result");
    Check(state.words == std::array<std::uint32_t, 5>{0, 2, 1, 1, 1} &&
              state.byte_14 == 1 && runtime.atomic_value == 1200.0f &&
              runtime.effect_value == 1200.0f,
          "default settings fields");
    Check(std::bit_cast<std::uint32_t>(runtime.levels[0]) == 0x3e99999a &&
              std::bit_cast<std::uint32_t>(runtime.levels[1]) == 0x3f4ccccd &&
              std::bit_cast<std::uint32_t>(runtime.levels[2]) == 0x3f4ccccd,
          "default level bit patterns");
    Check(runtime.manager_calls == 3 &&
              runtime.calls == std::vector<std::string>{
                  "manager", "set:0", "manager", "set:1", "manager",
                  "set:2", "atomic", "available", "effect"},
          "empty collections have no stream side effects");
}

void TestScaleOptionAndMissingEffectSystem() {
    Runtime runtime;
    pal4::GlobalDataSettingsState state{};
    runtime.ui_result = 0x3456;
    Check(pal4::GlobalDataSettings::ApplyScaleOption(state, 42, runtime) ==
              0x3456 && state.words[3] == 42 &&
              runtime.atomic_value == 1200.0f &&
              runtime.effect_value == 1200.0f &&
              runtime.calls == std::vector<std::string>{
                  "atomic", "available", "effect"},
          "nonzero option stores original value and forwards effect result");
    runtime.calls.clear();
    runtime.has_effect_system = false;
    bool threw = false;
    try {
        static_cast<void>(pal4::GlobalDataSettings::ApplyScaleOption(
            state, 0, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.words[3] == 0 &&
              runtime.atomic_value == 1800.0f &&
              runtime.calls == std::vector<std::string>{
                  "atomic", "available"},
          "missing effect system throws after the Atomic update");
}

}  // namespace

int main() {
    try {
        TestSaveAndLoad();
        TestDefaultsAndEmptyCollections();
        TestScaleOptionAndMissingEffectSystem();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
