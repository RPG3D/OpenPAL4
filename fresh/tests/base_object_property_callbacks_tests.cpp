#include "pal4/base_object_property_callbacks.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace {

using Event = std::pair<char, std::uint32_t>;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectPropertyCallbacksRuntime {
    std::vector<Event> events;
    std::uint32_t property_result{0x77u};
    std::uint32_t final_result{0x1234u};

    std::uint32_t GetProperty(pal4::BaseObjectState&,
                              std::uint32_t offset) override {
        Check(offset == 0x564u,
              "property getter calls virtual slot on character subobject");
        events.emplace_back('G', offset);
        return property_result;
    }
    void RegisterCountCallback(pal4::BaseObjectState&,
                               std::uint32_t offset) override {
        events.emplace_back('C', offset);
    }
    void RegisterByIndexCallback(pal4::BaseObjectState&,
                                 std::uint32_t offset) override {
        events.emplace_back('I', offset);
    }
    void RegisterByIdCallback(pal4::BaseObjectState&,
                              std::uint32_t offset) override {
        events.emplace_back('D', offset);
    }
    void RegisterActiveCallback(pal4::BaseObjectState&,
                                std::uint32_t offset) override {
        events.emplace_back('A', offset);
    }
    std::array<std::uint32_t, 3> ConvertZeroToInt64() override {
        events.emplace_back('Z', 0u);
        return {0x11u, 0x22u, 0x33u};
    }
    std::uint32_t SetInitialProperty(
        pal4::BaseObjectState& state, std::uint32_t offset,
        std::uint32_t id, std::uint32_t constant,
        const std::array<std::uint32_t, 3>& converted,
        std::uint32_t zero, std::uint32_t property_bits) override {
        Check(offset == 0x564u && id == 0x55u &&
                  constant == 0x41873333u &&
                  converted == std::array<std::uint32_t, 3>{
                                   0x11u, 0x22u, 0x33u} &&
                  zero == 0u && property_bits == 0xdeadbeefu &&
                  state.Read(0xdcu) == 0xdeadbeefu,
              "initializer forwards property id, constant, converter and raw bits");
        events.emplace_back('S', id);
        return final_result;
    }
};

void CheckGetProperty() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    Check(pal4::BaseObjectPropertyCallbacks::GetProperty(state, runtime) ==
              0x77u &&
              runtime.events == std::vector<Event>{{'G', 0x564u}},
          "getter forwards virtual result without modification");
}

void CheckInitialize() {
    pal4::BaseObjectState state{};
    state.Write(0xdcu, 0xdeadbeefu);
    Runtime runtime;
    Check(pal4::BaseObjectPropertyCallbacks::Initialize(
              state, 0x55u, runtime) == 0x1234u &&
              runtime.events ==
                  std::vector<Event>{{'G', 0x564u}, {'C', 0x564u},
                                     {'I', 0x564u}, {'D', 0x564u},
                                     {'A', 0x564u}, {'Z', 0u},
                                     {'S', 0x55u}},
          "initializer reads property, registers four callbacks and sets value");
    Check(state.Read(0xdcu) == 0xdeadbeefu,
          "initialization does not rewrite the source property bits");
}

}  // namespace

int main() {
    CheckGetProperty();
    CheckInitialize();
}
