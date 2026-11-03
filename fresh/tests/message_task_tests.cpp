#include "pal4/message_task.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMessageTaskRuntime {
    std::vector<std::string> calls;
    std::int32_t configured_id = -1;
    std::uint32_t last_text = 0u;
    std::uint32_t ResolveFacet(
        const std::uint32_t object,
        const std::string_view class_name) override {
        Check(object == 0x5000u && class_name == "MessageTask",
              "facet lookup receives object and class name");
        calls.emplace_back("resolve");
        return 0x6000u;
    }
    std::int32_t ReadConfiguredId(
        const std::uint32_t object, const std::string_view key,
        const std::int32_t default_value) override {
        Check(object == 0x5000u && key == "ID" && default_value == -1,
              "ID read uses owner config and -1 fallback");
        calls.emplace_back("id");
        return configured_id;
    }
    void Send(const std::uint32_t facet, const std::int32_t id,
              const std::uint32_t text_address) override {
        Check(facet == 0x6000u && id == configured_id,
              "message dispatch receives resolved facet and ID");
        calls.emplace_back("send");
        last_text = text_address;
    }
};

}  // namespace

int main() {
    pal4::MessageTaskState state{};
    state.words[16u] = 0x5000u;
    state.words[19u] = 0x7000u;
    Runtime runtime;
    runtime.configured_id = 42;
    Check(pal4::MessageTask::Start(state, runtime) == 1u &&
              state.words[22u] == 0x6000u &&
              runtime.last_text == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                                   "resolve", "id", "send"},
          "start resolves, reads ID and sends nonempty text");
    Check(pal4::MessageTask::Reset(state) == 1u &&
              state.words[22u] == 0u && state.words[19u] == 0x7000u,
          "reset clears facet without touching message string");

    state.words[19u] = 0u;
    runtime.calls.clear();
    runtime.configured_id = -1;
    Check(pal4::MessageTask::Start(state, runtime) == 1u &&
              runtime.last_text == pal4::MessageTask::kEmptyTextAddress &&
              runtime.calls == std::vector<std::string>{
                                   "resolve", "id", "send"},
          "missing message uses native static empty string");
}
