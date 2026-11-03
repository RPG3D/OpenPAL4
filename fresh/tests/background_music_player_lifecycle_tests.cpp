#include "pal4/background_music_player_lifecycle.h"

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

struct SingletonRuntime final : pal4::IBackgroundMusicSingletonRuntime {
    pal4::BackgroundMusicSingletonState* state = nullptr;
    std::vector<Call> calls;
    std::uint32_t registration_result = 0x9000u;

    std::uint32_t GetSingletonManager() override {
        Check(state != nullptr &&
                  (state->initialization_bits & 1u) != 0u,
              "music singleton guard is set before manager lookup");
        calls.push_back({"manager"});
        return 0x8000u;
    }
    std::uint32_t Register(const std::uint32_t manager, const char* name,
                           const std::uint32_t id,
                           const std::uint32_t factory,
                           const std::uint32_t destructor) override {
        Check(manager == 0x8000u &&
                  std::string_view(name) == "palBackgroundMusicPlayer" &&
                  id == 1044u && factory == 0x513cd0u &&
                  destructor == 0x513d30u,
              "music singleton registration identity matches IDA");
        calls.push_back({"register", factory, destructor});
        return registration_result;
    }
};

struct LifecycleRuntime final : pal4::IBackgroundMusicLifecycleRuntime {
    std::vector<Call> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t allocation = 0x5000u;

    std::uint32_t Allocate(const std::uint32_t size) override {
        calls.push_back({"allocate", size});
        return allocation;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        calls.push_back({"free", address});
        return address + 0x10u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back({"read-word", address});
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back({"write-word", address, value});
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        calls.push_back({"read-byte", address});
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        calls.push_back({"write-byte", address, value});
        bytes[address] = value;
    }
};

void SingletonRegistration() {
    pal4::BackgroundMusicSingletonState state{0xa0u, 0u};
    pal4::BackgroundMusicSingleton singleton(state);
    SingletonRuntime runtime;
    runtime.state = &state;
    Check(singleton.GetInstance(runtime) == 0x9000u &&
              state.initialization_bits == 0xa1u &&
              state.instance == 0x9000u &&
              singleton.GetInstance(runtime) == 0x9000u &&
              runtime.calls == std::vector<Call>{
                  {"manager"}, {"register", 0x513cd0u, 0x513d30u}},
          "music singleton preserves guard bits and caches registration");

    pal4::BackgroundMusicSingletonState failed_state{};
    pal4::BackgroundMusicSingleton failed(failed_state);
    SingletonRuntime failed_runtime;
    failed_runtime.state = &failed_state;
    failed_runtime.registration_result = 0u;
    Check(failed.GetInstance(failed_runtime) == 0u &&
              failed.GetInstance(failed_runtime) == 0u &&
              failed_runtime.calls.size() == 2u,
          "failed music registration is still guarded");
}

void ConstructionAndAllocationFailure() {
    LifecycleRuntime runtime;
    Check(pal4::BackgroundMusicPlayerLifecycle::Create(runtime) == 0x5000u &&
              runtime.calls.front() == Call{"allocate", 0x40u},
          "music player factory allocates exactly 64 bytes");
    std::vector<Call> expected{{"allocate", 0x40u}};
    for (const auto offset : {0u, 16u, 32u}) {
        expected.push_back({"write-byte", 0x5000u + offset, 0u});
        for (const auto word : {4u, 8u, 12u})
            expected.push_back({"write-word", 0x5000u + offset + word, 0u});
    }
    for (const auto offset : {48u, 52u, 56u, 60u})
        expected.push_back({"write-word", 0x5000u + offset, 0u});
    Check(runtime.calls == expected && runtime.bytes.at(0x5000u) == 0u &&
              runtime.bytes.at(0x5010u) == 0u &&
              runtime.bytes.at(0x5020u) == 0u,
          "music constructor initializes three text states and trailing words");

    LifecycleRuntime direct;
    Check(pal4::BackgroundMusicPlayerLifecycle::Construct(
              0x6000u, direct) == 0x6000u &&
              direct.calls.size() == 16u &&
              direct.calls.front() == Call{"write-byte", 0x6000u},
          "direct music constructor returns original object");

    LifecycleRuntime failed;
    failed.allocation = 0u;
    Check(pal4::BackgroundMusicPlayerLifecycle::Create(failed) == 0u &&
              failed.calls == std::vector<Call>{{"allocate", 0x40u}},
          "music player allocation failure skips construction");
}

void CleanupAndDestruction() {
    LifecycleRuntime runtime;
    runtime.words[0x5024u] = 0x1001u;
    runtime.words[0x5014u] = 0x2001u;
    runtime.words[0x5004u] = 0x3001u;
    runtime.bytes[0x1000u] = 0u;
    runtime.bytes[0x2000u] = 2u;
    runtime.bytes[0x3000u] = 0xffu;
    Check(pal4::BackgroundMusicPlayerLifecycle::CleanupResources(
              0x5000u, runtime) == 0x3010u &&
              runtime.calls == std::vector<Call>{
                  {"read-word", 0x5024u}, {"read-byte", 0x1000u},
                  {"free", 0x1000u},
                  {"write-word", 0x5024u}, {"write-word", 0x5028u},
                  {"write-word", 0x502cu},
                  {"read-word", 0x5014u}, {"read-byte", 0x2000u},
                  {"write-byte", 0x2000u, 1u},
                  {"write-word", 0x5014u}, {"write-word", 0x5018u},
                  {"write-word", 0x501cu},
                  {"read-word", 0x5004u}, {"read-byte", 0x3000u},
                  {"free", 0x3000u},
                  {"write-word", 0x5004u}, {"write-word", 0x5008u},
                  {"write-word", 0x500cu}},
          "music text buffers release in reverse order and clear all words");
    Check(runtime.bytes.at(0x2000u) == 1u &&
              runtime.words.at(0x5024u) == 0u &&
              runtime.words.at(0x5014u) == 0u &&
              runtime.words.at(0x5004u) == 0u,
          "shared buffer decrements its reference and text pointers reset");

    LifecycleRuntime destroyed;
    destroyed.words[0x5024u] = 0u;
    destroyed.words[0x5014u] = 0u;
    destroyed.words[0x5004u] = 0u;
    Check(pal4::BackgroundMusicPlayerLifecycle::Destroy(
              0x5000u, destroyed) == 0x5010u &&
              destroyed.calls.back() == Call{"free", 0x5000u},
          "player object is freed after its three text states");
    LifecycleRuntime null_runtime;
    static_cast<void>(pal4::BackgroundMusicPlayerLifecycle::Destroy(
        0u, null_runtime));
    Check(null_runtime.calls.empty(), "null player destruction is a no-op");
}

}  // namespace

int main() {
    try {
        SingletonRegistration();
        ConstructionAndAllocationFailure();
        CleanupAndDestruction();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
