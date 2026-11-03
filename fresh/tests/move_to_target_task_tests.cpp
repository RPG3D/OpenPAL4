#include "pal4/move_to_target_task.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMoveToTargetTaskRuntime {
    std::vector<std::string> calls;
    std::uint8_t count = 0u;
    std::uint8_t stored_count = 0u;
    std::uint32_t freed = 0u;
    bool found = true;
    std::uint8_t old_flag = 0x72u;
    std::array<std::uint32_t, 3u> last_position{};
    bool last_relative = false;

    void ConstructBase(pal4::MoveToTargetTaskState& state) override {
        calls.emplace_back("construct");
        state.words[0u] = 0xdeadbeefu;
    }
    void DestroyBase(pal4::MoveToTargetTaskState& state) override {
        Check(state.words[0u] == pal4::MoveToTargetTask::kVtable &&
                  state.words[28u] == 0u && state.words[29u] == 0u &&
                  state.words[30u] == 0u, "base sees cleared derived storage");
        calls.emplace_back("destroy");
    }
    std::uint8_t RefCount(std::uint32_t data) override {
        Check(data == 0x7001u, "refcount reads string data");
        calls.emplace_back("count");
        return count;
    }
    void SetRefCount(std::uint32_t data, std::uint8_t value) override {
        Check(data == 0x7001u, "refcount writes string data");
        calls.emplace_back("decrement");
        stored_count = value;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
    bool ResolveFacet(std::uint32_t object, std::string_view name,
                      std::uint32_t& facet) override {
        Check(object == 0x5000u && name == "LocalModelTask",
              "local model lookup uses owner and class");
        calls.emplace_back("resolve");
        facet = found ? 0x6000u : 0u;
        return found;
    }
    std::uint8_t ReadFacetFlag(std::uint32_t facet) override {
        Check(facet == 0x6000u, "reads local model facet");
        calls.emplace_back("read");
        return old_flag;
    }
    void WriteFacetFlag(std::uint32_t facet, std::uint8_t flag) override {
        Check(facet == 0x6000u &&
                  (flag == 0u || flag == old_flag),
              "local model flag disabled or restored");
        calls.emplace_back("flag:" + std::to_string(flag));
    }
    void SetTarget(std::uint32_t facet,
                   const std::array<std::uint32_t, 3u>& value) override {
        Check(facet == 0x6100u && value ==
                  (std::array<std::uint32_t, 3u>{1u, 2u, 3u}),
              "target triplet at +84");
        calls.emplace_back("target");
    }
    void SetOrigin(std::uint32_t facet,
                   const std::array<std::uint32_t, 3u>& value) override {
        Check(facet == 0x6100u && value ==
                  (std::array<std::uint32_t, 3u>{4u, 5u, 6u}),
              "origin triplet at +96");
        calls.emplace_back("origin");
    }
    void Enable(std::uint32_t facet, bool enabled) override {
        Check(facet == 0x6100u, "enable move facet");
        calls.emplace_back(enabled ? "enable" : "disable");
    }
    std::uint32_t SetPosition(
        std::uint32_t facet, const std::array<std::uint32_t, 3u>& value,
        bool relative) override {
        Check(facet == 0x6100u, "set move facet position");
        calls.emplace_back(relative ? "relative" : "absolute");
        last_position = value;
        last_relative = relative;
        return 0u;
    }
};

}  // namespace

int main() {
    pal4::MoveToTargetTaskState state{};
    state.words.fill(0xababababu);
    Runtime runtime;
    Check(&pal4::MoveToTargetTask::Construct(state, 0x35u, runtime) == &state &&
              state.words[0u] == pal4::MoveToTargetTask::kVtable &&
              state.words[27u] == 0xababab35u &&
              state.words[28u] == 0u && state.words[29u] == 0u &&
              state.words[30u] == 0u && state.words[31u] == 0u &&
              runtime.calls == std::vector<std::string>{"construct"},
          "constructor preserves stack byte and clears four words");
    Check(pal4::MoveToTargetTask::ClassName() == "MoveToTargetTask",
          "class name matches IDA literal");

    state.words[16u] = 0x5000u;
    state.words[32u] = 0xabcdef00u;
    runtime.calls.clear();
    Check(pal4::MoveToTargetTask::Start(state, runtime) == 1u &&
              state.words[33u] == 0x6000u &&
              state.words[32u] == 0xabcdef72u &&
              runtime.calls == std::vector<std::string>{
                                   "resolve", "read", "flag:0"},
          "start saves and disables local model state");

    state.words[31u] = 0x6100u;
    state.words[18u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[19u] = std::bit_cast<std::uint32_t>(-4.0f);
    state.words[20u] = std::bit_cast<std::uint32_t>(6.0f);
    for (unsigned index = 0; index < 6u; ++index)
        state.words[21u + index] = index + 1u;
    runtime.calls.clear();
    Check(pal4::MoveToTargetTask::Update(state, 0.5f, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                                   "target", "origin", "enable", "relative"} &&
              runtime.last_position ==
                  (std::array<std::uint32_t, 3u>{
                      std::bit_cast<std::uint32_t>(1.0f),
                      std::bit_cast<std::uint32_t>(-2.0f),
                      std::bit_cast<std::uint32_t>(3.0f)}),
          "update scales movement and dispatches four ordered facet calls");

    runtime.calls.clear();
    Check(pal4::MoveToTargetTask::Stop(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                                   "absolute", "disable", "flag:114"} &&
              runtime.last_position ==
                  (std::array<std::uint32_t, 3u>{1u, 2u, 3u}) &&
              state.words[31u] == 0u && state.words[33u] == 0u,
          "stop restores target and local model flag");

    runtime.found = false;
    runtime.calls.clear();
    Check(pal4::MoveToTargetTask::Start(state, runtime) == 1u &&
              state.words[33u] == 0u &&
              runtime.calls == std::vector<std::string>{"resolve"},
          "missing local model does not touch flag");
    state.words[31u] = 0x6100u;
    runtime.calls.clear();
    Check(pal4::MoveToTargetTask::Stop(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                                   "absolute", "disable"} &&
              state.words[31u] == 0u,
          "stop without local model skips flag restoration");

    state.words[28u] = 0x7001u;
    state.words[29u] = 5u;
    state.words[30u] = 9u;
    state.words[31u] = 0x6100u;
    runtime.count = 2u;
    runtime.calls.clear();
    Check(&pal4::MoveToTargetTask::Destroy(state, runtime) == &state &&
              runtime.stored_count == 1u &&
              runtime.calls == std::vector<std::string>{
                                   "count", "decrement", "destroy"} &&
              state.words[31u] == 0x6100u,
          "shared string decremented; fourth word preserved");

    state.words[28u] = 0x7001u;
    runtime.count = 0xffu;
    runtime.calls.clear();
    (void)pal4::MoveToTargetTask::Destroy(state, runtime);
    Check(runtime.freed == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                                   "count", "free", "destroy"},
          "sentinel string allocation freed");

    state.words[28u] = 0x7001u;
    runtime.count = 0u;
    runtime.freed = 0u;
    (void)pal4::MoveToTargetTask::Destroy(state, runtime);
    Check(runtime.freed == 0x7000u,
          "unique string allocation freed");
}
