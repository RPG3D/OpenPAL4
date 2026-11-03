#include "pal4/base_object_effect_list.h"

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

struct Runtime final : pal4::IBaseObjectEffectListRuntime {
    std::vector<Event> events;

    void AddOrReplaceComponent(pal4::BaseObjectState&,
                               std::uint32_t animation,
                               std::uint32_t component) override {
        Check(animation == 0x170u &&
                  (component == 0x210u || component == 0x208u),
              "direct addition uses animation and selected component offsets");
        events.emplace_back('A', component);
    }
    std::uint32_t QueryComponent(pal4::BaseObjectState&,
                                 std::uint32_t component) override {
        Check(component == 0x210u || component == 0x208u,
              "component query invokes virtual slot +4");
        events.emplace_back('Q', component);
        return component + 0x1000u;
    }
    void RemoveById(pal4::BaseObjectState&,
                    std::uint32_t animation,
                    std::uint32_t id) override {
        Check(animation == 0x170u &&
                  (id == 0x1210u || id == 0x1208u),
              "resolved removal uses queried component value");
        events.emplace_back('R', id);
    }
};

void CheckAllVariants() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    Check(pal4::BaseObjectEffectList::UpdateFirst(state, runtime) == 1u &&
              pal4::BaseObjectEffectList::UpdateFirstResolved(
                  state, runtime) == 1u &&
              pal4::BaseObjectEffectList::UpdateSecond(
                  state, runtime) == 1u &&
              pal4::BaseObjectEffectList::UpdateSecondResolved(
                  state, runtime) == 1u,
          "all effect-list wrappers return one");
    Check(runtime.events == std::vector<Event>{
              {'A', 0x210u}, {'Q', 0x210u}, {'R', 0x1210u},
              {'A', 0x208u}, {'Q', 0x208u}, {'R', 0x1208u}},
          "component add/remove dispatch matches four IDA entries");
}

}  // namespace

int main() { CheckAllVariants(); }
