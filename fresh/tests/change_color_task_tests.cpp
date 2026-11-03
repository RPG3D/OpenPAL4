#include "pal4/change_color_task.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IChangeColorTaskRuntime {
    std::vector<const char*> calls;
    void ConstructBase(pal4::ChangeColorTaskState& state) override {
        calls.push_back("construct");
        state.words[1u] = 0x12345678u;
    }
    void DestroyBase(pal4::ChangeColorTaskState& state) override {
        Check(state.words[0u] == pal4::ChangeColorTask::kVtable,
              "derived vtable restored before base destruction");
        calls.push_back("destroy");
    }
};

}  // namespace

int main() {
    pal4::ChangeColorTaskState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    auto* const original = &state;
    Check(&pal4::ChangeColorTask::Construct(state, runtime) == original &&
              state.words[0u] == 0x84abe0u &&
              state.words[1u] == 0x12345678u &&
              state.words[34u] == 0xdeadbe00u,
          "base constructor precedes derived vtable and byte +136 clear");
    for (const auto index : {35u, 21u, 20u, 19u, 18u, 25u, 24u, 23u,
                             22u, 29u, 28u, 27u, 26u})
        Check(state.words[index] == 0u, "all IDA constructor words cleared");
    Check(state.words[17u] == 0xdeadbeefu &&
              state.words[30u] == 0xdeadbeefu,
          "unwritten state remains untouched");
    state.words[0u] = 0u;
    Check(&pal4::ChangeColorTask::Destroy(state, runtime) == original &&
              state.words[0u] == 0x84abe0u &&
              runtime.calls.size() == 2u &&
              std::string_view(runtime.calls[0u]) == "construct" &&
              std::string_view(runtime.calls[1u]) == "destroy",
          "destruction restores vtable before base cleanup");
    Check(pal4::ChangeColorTask::ClassName() == "ChangeColorTask",
          "class name matches IDA string at 0x8c582c");
}
