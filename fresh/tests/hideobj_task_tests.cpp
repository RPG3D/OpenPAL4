#include "pal4/hideobj_task.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct ObjectRuntime final : pal4::IHideobjTaskRuntime {
    std::vector<std::string> calls;
    void ConstructBase(pal4::HideobjTaskState& state) override {
        calls.emplace_back("construct");
        state.words[1u] = 0x12345678u;
    }
    void DestroyBase(pal4::HideobjTaskState& state) override {
        Check(state.words[0u] == pal4::HideobjTask::kVtable,
              "derived vtable restored before base destructor");
        calls.emplace_back("destroy");
    }
};

struct FacetRuntime final : pal4::IHideobjFacetRuntime {
    std::vector<std::string> calls;
    std::uint32_t ResolveFacet(
        const std::uint32_t object,
        const std::string_view class_name) override {
        Check(object == 0x5000u && class_name == "HideobjTask",
              "lookup uses owner +64 and native class name");
        calls.emplace_back("resolve");
        return 0x6000u;
    }
    void Hide(const std::uint32_t facet) override {
        Check(facet == 0x6000u, "hide dispatches resolved facet");
        calls.emplace_back("hide");
    }
};

}  // namespace

int main() {
    pal4::HideobjTaskState state{};
    state.words.fill(0xdeadbeefu);
    ObjectRuntime object;
    auto* const original = &state;
    Check(&pal4::HideobjTask::Construct(state, object) == original &&
              state.words[0u] == 0x84ac60u &&
              state.words[1u] == 0x12345678u &&
              state.words[18u] == 0u &&
              state.words[17u] == 0xdeadbeefu &&
              pal4::HideobjTask::ClassName() == "HideobjTask",
          "factory size, base order, vtable and class name");

    state.words[16u] = 0x5000u;
    FacetRuntime facet;
    Check(pal4::HideobjTask::Start(state, facet) == 1u &&
              state.words[18u] == 0x6000u &&
              facet.calls == std::vector<std::string>{"resolve", "hide"},
          "start resolves then hides facet");

    state.words[0u] = 0u;
    Check(&pal4::HideobjTask::Destroy(state, object) == original &&
              object.calls == std::vector<std::string>{"construct", "destroy"} &&
              state.words[0u] == 0x84ac60u,
          "destruction restores derived vtable before base cleanup");
}
