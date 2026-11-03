#include "pal4/game_data_property_field_names.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Event {
    char kind;
    std::uint32_t address;
    std::string_view text{};
    std::uint8_t value{};

    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::IGameDataPropertyFieldNamesRuntime {
    std::vector<Event> events;
    std::map<std::uint32_t, std::string_view> names;
    std::uint32_t fail_address = 0;
    int register_result = -23;

    std::uint8_t InitialSeed() override { return 0x5a; }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.push_back({'B', address, {}, value});
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(value == 0, "text storage words start at zero");
        events.push_back({'W', address});
    }
    void ResetText(std::uint32_t address) override {
        events.push_back({'R', address});
    }
    void AssignText(std::uint32_t address,
                    std::string_view text) override {
        events.push_back({'A', address, text});
        names[address] = text;
    }
    bool AllocateText(std::uint32_t address,
                      std::string_view text) override {
        events.push_back({'L', address, text});
        if (address == fail_address) return false;
        names[address] = text;
        return true;
    }
    void DestroyText(std::uint32_t address) override {
        events.push_back({'D', address});
    }
    int RegisterAtexit(std::uint32_t callback) override {
        events.push_back({'X', callback});
        return register_result;
    }
};

struct ExpectedName {
    std::string_view text;
    char method;
};

constexpr std::array kCore = {
    ExpectedName{"PA_NAME", 'L'},
    ExpectedName{"PA_DESCRIPTION", 'L'},
    ExpectedName{"PA_PROPERTY", 'L'},
    ExpectedName{"PA_ICON", 'L'},
    ExpectedName{"PA_MODEL", 'L'},
    ExpectedName{"PA_TEXTURE", 'L'},
    ExpectedName{"PA_ATTACHED_EFFECT", 'A'},
};

constexpr std::array kExtra = {
    ExpectedName{"PA_PRICE", 'A'},
    ExpectedName{"PA_ATTACHED_SKILL", 'A'},
    ExpectedName{"PRIZE_LEVEL", 'A'},
    ExpectedName{"PA_ATK_TYPE", 'L'},
    ExpectedName{"CANDROP", 'L'},
};

void CheckGroup(pal4::PropertyFieldGroup group, std::uint32_t base,
                std::uint32_t destructor,
                const std::vector<ExpectedName>& expected) {
    Runtime runtime;
    Check(pal4::GameDataPropertyFieldNames::InitializeAndRegister(
              group, runtime) == -23,
          "initializer forwards CRT registration result");
    std::vector<Event> events;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto address = base + static_cast<std::uint32_t>(i) * 16;
        events.push_back({'B', address, {}, 0x5a});
        if (expected[i].method == 'A' && group ==
                                             pal4::PropertyFieldGroup::Extra) {
            for (std::uint32_t offset : {4u, 8u, 12u})
                events.push_back({'W', address + offset});
        } else {
            events.push_back({'R', address});
        }
        events.push_back({expected[i].method, address, expected[i].text});
        Check(runtime.names[address] == expected[i].text,
              "field name uses the verified IDA literal");
    }
    events.push_back({'X', destructor});
    Check(runtime.events == events,
          "initialization uses the verified addresses, modes and order");

    runtime.events.clear();
    pal4::GameDataPropertyFieldNames::Destroy(group, runtime);
    Check(runtime.events.size() == expected.size(),
          "destructor visits every text");
    for (std::size_t i = 0; i < expected.size(); ++i)
        Check(runtime.events[i] == Event{
                  'D', base + static_cast<std::uint32_t>(
                                  expected.size() - i - 1) * 16},
              "destructor visits texts in reverse address order");
}

void CheckAllocationFailureContinues() {
    Runtime runtime;
    runtime.fail_address = 0x8ef080 + 2 * 16;
    pal4::GameDataPropertyFieldNames::Initialize(
        pal4::PropertyFieldGroup::Core, runtime);
    Check(!runtime.names.contains(runtime.fail_address) &&
              runtime.names[0x8ef080 + 3 * 16] == "PA_ICON" &&
              runtime.names[0x8ef080 + 6 * 16] ==
                  "PA_ATTACHED_EFFECT",
          "failed allocation leaves that field empty and later fields run");
    for (const auto& event : runtime.events)
        Check(event.kind != 'X',
              "plain initialization does not register an exit callback");
}

}  // namespace

int main() {
    try {
        CheckGroup(pal4::PropertyFieldGroup::Core, 0x8ef080, 0x4e4c10,
                   {kCore.begin(), kCore.end()});
        CheckGroup(pal4::PropertyFieldGroup::Extra, 0x8ef030, 0x4e4dd0,
                   {kExtra.begin(), kExtra.end()});
        CheckAllocationFailureContinues();
        std::cout << "game_data_property_field_names_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
