#include "pal4/game_object_animation_event_data.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

std::uint32_t Word(const pal4::AnimationEventData& event,
                   std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, event.data() + offset, sizeof(value));
    return value;
}

class Runtime final : public pal4::IGameObjectAnimationEventStringRuntime {
public:
    std::vector<int> fields;
    void CopyStringField(
        std::span<std::byte, 64> destination,
        std::span<const std::byte, 64> source) override {
        fields.push_back(std::to_integer<int>(source[0]));
        std::copy(source.begin(), source.end(), destination.begin());
    }
};

void ResetAndNormalize() {
    pal4::AnimationEventData event;
    event.fill(std::byte{0x7f});
    Check(&pal4::GameObjectAnimationEventData::Reset(event) == &event,
          "reset returns event identity");
    for (const auto offset : {0u, 64u, 128u, 204u})
        Check(event[offset] == std::byte{}, "reset clears each name byte");
    Check(event[1] == std::byte{0x7f} &&
              event[203] == std::byte{0xbf} &&
              event[205] == std::byte{0x7f} &&
              Word(event, 192) == 0xbf800000u &&
              Word(event, 196) == 0 &&
              Word(event, 200) == 0xbf800000u,
          "reset preserves unrelated bytes and writes negative-one time bits");
    const std::uint32_t trigger = 0x3f400000u;
    std::memcpy(event.data() + 192, &trigger, 4);
    Check(pal4::GameObjectAnimationEventData::NormalizeTime(event) == trigger &&
              Word(event, 196) == 0 && Word(event, 200) == trigger,
          "normalize returns trigger bits and clears fired flag");
}

void CopyFieldsAndSnapshot() {
    pal4::AnimationEventData source;
    source.fill(std::byte{0x3a});
    source[0] = std::byte{1};
    source[64] = std::byte{2};
    source[128] = std::byte{3};
    source[192] = std::byte{4};
    source[331] = std::byte{5};
    pal4::AnimationEventData destination;
    destination.fill(std::byte{0x7f});
    pal4::AnimationEventData scratch{};
    Runtime runtime;
    Check(&pal4::GameObjectAnimationEventData::Copy(
              destination, scratch, source, runtime) == &scratch &&
              runtime.fields == std::vector<int>{1, 2, 3} &&
              destination == source && scratch == destination,
          "copy invokes three string fields and snapshots complete 332 bytes");
}

}  // namespace

int main() {
    ResetAndNormalize();
    CopyFieldsAndSnapshot();
    std::cout << "game_object_animation_event_data_tests passed\n";
}
