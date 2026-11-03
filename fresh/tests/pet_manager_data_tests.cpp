#include "pal4/pet_manager.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(float actual, float expected) {
    return std::abs(actual - expected) < 0.0001f;
}

struct Runtime final : pal4::PetManagerRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    pal4::PetVector3 position{};
    std::uint32_t facing_source{};
    std::uint32_t property_result{0x7777u};
    std::uint32_t facing_result{0x8888u};
    bool change_source_in_position{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t SetProperty(std::uint32_t object,
                              std::uint32_t value) override {
        Check(object == 0x5000u && value == 0x1234u,
              "property thunk forwards this and the argument");
        events.emplace_back("property");
        return property_result;
    }
    double GetDoubleProperty(std::uint32_t source) override {
        Check(source == 0x9000u, "double property receives source object");
        events.emplace_back("double-property");
        return 2.25;
    }
    void SetPetPosition(std::uint32_t object,
                        const pal4::PetVector3& value,
                        std::uint32_t mode) override {
        Check(object == 0x5000u && mode == 0u,
              "position virtual receives object and zero mode");
        position = value;
        events.emplace_back("position");
        if (change_source_in_position) words[0x5658u] = 0xa000u;
    }
    std::uint32_t SetPetFacing(std::uint32_t object,
                               std::uint32_t source_vector,
                               std::uint32_t mode) override {
        Check(object == 0x5000u && mode == 0u,
              "facing virtual receives object and zero mode");
        facing_source = source_vector;
        events.emplace_back("facing");
        return facing_result;
    }
};

void SetSourceCoordinates(Runtime& runtime) {
    runtime.words[0x90acu] = std::bit_cast<std::uint32_t>(4.0f);
    runtime.words[0x90b0u] = std::bit_cast<std::uint32_t>(10.0f);
    runtime.words[0x90b4u] = std::bit_cast<std::uint32_t>(6.0f);
}

void PropertyThunkAndDirectInitialization() {
    Runtime runtime;
    Check(pal4::PetManager::ForwardProperty(
              0x5000u, 0x1234u, runtime) == 0x7777u &&
              runtime.events == std::vector<std::string>{"property"},
          "direct thunk forwards result without extra work");

    SetSourceCoordinates(runtime);
    runtime.words[0x5668u] = 0xaaaaaaaau;
    runtime.events.clear();
    Check(pal4::PetManager::InitializePetData(
              0x5000u, 0x9000u, runtime) == 0x8888u,
          "direct initialization returns final facing result");
    Check(runtime.words[0x5658u] == 0x9000u &&
              runtime.words[0x565cu] == 0u &&
              runtime.words[0x5660u] == 0u &&
              runtime.words[0x5664u] ==
                  std::bit_cast<std::uint32_t>(2.25f) &&
              runtime.words[0x5668u] == 0xaaaaaaaau,
          "source, state and float property writes preserve adjacent word");
    Check(Near(runtime.position.x, 4.0f) &&
              Near(runtime.position.y, 12.25f) &&
              Near(runtime.position.z, 6.0f) &&
              runtime.facing_source == 0x90a0u,
          "position adds double property to source Y and facing uses +0xa0");
    Check(runtime.events == std::vector<std::string>{
              "write:22104", "write:22108", "write:22112",
              "double-property", "write:22116", "read:22104",
              "read:37036", "read:37040", "read:37044",
              "position", "read:22104", "facing"},
          "direct initialization preserves call and memory-read ordering");
}

void ConditionalInitialization() {
    Runtime runtime;
    Check(pal4::PetManager::InitializeStoredPetData(
              0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"read:22104"},
          "absent stored source leaves all other fields unchanged");

    SetSourceCoordinates(runtime);
    runtime.words[0x5658u] = 0x9000u;
    runtime.change_source_in_position = true;
    runtime.events.clear();
    Check(pal4::PetManager::InitializeStoredPetData(
              0x5000u, runtime) == 0x8888u &&
              runtime.facing_source == 0xa0a0u,
          "conditional version rereads stored source after position virtual");
    Check(runtime.events.front() == "read:22104" &&
              runtime.events[1] == "write:22108" &&
              runtime.events.back() == "facing",
          "conditional source check precedes the shared initialization body");
}

void ResetState() {
    Runtime runtime;
    runtime.words[0x5654u] = 0x7777u;
    runtime.words[0x5658u] = 0x9000u;
    runtime.words[0x565cu] = 3u;
    runtime.words[0x5660u] = 0u;
    Check(pal4::PetManager::ResetState(0x5000u, runtime) == 0u &&
              runtime.words[0x5654u] == 0x7777u &&
              runtime.words[0x5658u] == 0u &&
              runtime.words[0x565cu] == 0u &&
              runtime.words[0x5660u] == 1u &&
              runtime.words[0x5668u] == 0x41a00000u,
          "state reset changes exactly the observed fields");
    Check(runtime.events == std::vector<std::string>{
              "write:22112", "write:22104", "write:22108", "write:22120"},
          "state reset follows IDA store order");
}

}  // namespace

int main() {
    PropertyThunkAndDirectInitialization();
    ConditionalInitialization();
    ResetState();
    std::cout << "3 pet data test groups passed\n";
}
