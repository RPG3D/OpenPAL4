#include "pal4/player_manager_object.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPlayerManagerVectorRuntime {
    pal4::PlayerManagerObjectState& state;
    bool constructed{};
    bool destroyed{};

    explicit Runtime(pal4::PlayerManagerObjectState& image) : state(image) {}
    void ConstructVector(std::uint32_t object,
        std::uint32_t stride, std::uint32_t count,
        std::uint32_t constructor, std::uint32_t destructor) override {
        Check(object == 0x5000u && stride == 0xb14u && count == 4u &&
                  constructor == 0x43e6a0u && destructor == 0x43e890u &&
                  state.Word(0x2c50u / 4u) == 0xa5a5a5a5u,
              "constructor passes exact CRT iterator arguments before clear");
        constructed = true;
    }
    void DestroyVector(std::uint32_t object,
        std::uint32_t stride, std::uint32_t count,
        std::uint32_t destructor) override {
        Check(object == 0x5000u && stride == 0xb14u && count == 4u &&
                  destructor == 0x43e890u,
              "destructor passes exact CRT iterator arguments");
        destroyed = true;
    }
};

struct CallbackRuntime final : pal4::PlayerManagerCallbackRuntimeStub {
    std::vector<std::uint32_t> slots;
    bool throw_on_third{};

    void Invoke(std::uint32_t callback, std::uint32_t slot,
                std::uint32_t context) override {
        Check(callback == 0x1234u && context == 0xabcdU &&
                  slot == 0x5000u + slots.size() * 0xb14u,
              "callback receives original function, slot and context");
        slots.push_back(slot);
        if (throw_on_third && slots.size() == 3u)
            throw std::runtime_error("callback failure");
    }
};

}  // namespace

int main() {
    pal4::PlayerManagerObjectState state{};
    state.Fill(0xa5a5a5a5u);
    Runtime runtime(state);
    Check(pal4::PlayerManagerObject::Construct(
              state, 0x5000u, runtime) == 0x5000u &&
              runtime.constructed && state.Word(0x2c50u / 4u) == 0u &&
              state.Word(0x2c4cu / 4u) == 0xa5a5a5a5u &&
              state.Word(0x2c54u / 4u) == 0xa5a5a5a5u,
          "manager constructor clears only observed post-array word");
    pal4::PlayerManagerObject::Destroy(0x5000u, runtime);
    Check(runtime.destroyed, "manager destructor invokes CRT iterator");
    for (std::int32_t index = 0; index < 4; ++index)
        Check(pal4::PlayerManagerObject::GetPlayerByIndex(0x5000u, index) ==
                  0x5000u + static_cast<std::uint32_t>(index) * 0xb14u,
              "valid indexes select the matching 0xb14-byte slot");
    Check(pal4::PlayerManagerObject::GetPlayerByIndex(0x5000u, 4) == 0u &&
          pal4::PlayerManagerObject::GetPlayerByIndex(0x5000u, 99) == 0u,
          "indexes at or beyond four return null");
    Check(pal4::PlayerManagerObject::GetPlayerByIndex(0x5000u, -1) ==
              0x5000u - 0xb14u,
          "negative index preserves the IDA signed upper-bound-only test");

    state.Fill(0xa5a5a5a5u);
    for (std::uint32_t mask = 0u; mask < 16u; ++mask) {
        for (std::uint32_t index = 0u; index < 4u; ++index)
            state.Word((index * 0xb14u + 0xb08u) / 4u) =
                (mask & (1u << index)) != 0u ? index + 1u : 0u;
        std::uint32_t expected = 0u;
        for (std::uint32_t index = 0u; index < 4u; ++index)
            expected += (mask & (1u << index)) != 0u;
        Check(pal4::PlayerManagerObject::GetActivePlayerCount(state) == expected,
              "count includes exactly the nonzero membership words");
    }

    for (std::uint32_t current = 0u; current < 4u; ++current) {
        state.Fill(0xa5a5a5a5u);
        state.Word(0x2c50u / 4u) = current;
        const auto returned = pal4::PlayerManagerObject::SetPlayerState(
            state, 0x12340000u + current);
        Check(returned == current * (0xb14u / 4u),
              "state setter returns final EAX slot-word offset");
        for (std::uint32_t index = 0u; index < 4u; ++index)
            Check(state.Word((index * 0xb14u + 0xb0cu) / 4u) ==
                      (index == current ? 0x12340000u + current : 0u),
                  "state setter clears four slots then writes current one");
        Check(state.Word(0x2c4cu / 4u) == 0xa5a5a5a5u &&
              state.Word(0x2c54u / 4u) == 0xa5a5a5a5u,
              "adjacent manager words stay intact");
    }
    CallbackRuntime callback;
    pal4::PlayerManagerObject::ForAllPlayers(
        0x5000u, 0x1234u, 0xabcdu, callback);
    Check(callback.slots == std::vector<std::uint32_t>{
              0x5000u, 0x5b14u, 0x6628u, 0x713cu},
          "callback visits all four slots regardless of membership state");
    CallbackRuntime failure;
    failure.throw_on_third = true;
    bool threw = false;
    try {
        pal4::PlayerManagerObject::ForAllPlayers(
            0x5000u, 0x1234u, 0xabcdu, failure);
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && failure.slots.size() == 3u,
          "callback exception stops iteration without a fourth call");
}
