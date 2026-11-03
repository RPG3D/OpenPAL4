#include "pal4/script_execution_context.h"
#include "pal4/script_wait_handler_base.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptHandlerRegistrationRuntime {
    std::vector<std::string> events;
    std::vector<std::size_t> allocations;
    pal4::ScriptWaitHandlerBaseState* reset = nullptr;
    pal4::ScriptWaitHandlerBaseState* deleted = nullptr;

    bool SameHandlerKey(const pal4::ScriptWaitHandlerBaseState& incoming,
                        const pal4::ScriptWaitHandlerBaseState& existing)
        override {
        events.emplace_back("compare");
        const auto incoming_size = incoming.text_end - incoming.text_data;
        const auto existing_size = existing.text_end - existing.text_data;
        return incoming_size == existing_size &&
               std::string_view(reinterpret_cast<const char*>(incoming.text_data),
                                static_cast<std::size_t>(incoming_size)) ==
                   std::string_view(reinterpret_cast<const char*>(existing.text_data),
                                    static_cast<std::size_t>(existing_size));
    }
    void ResetHandler(pal4::ScriptWaitHandlerBaseState& handler) override {
        events.emplace_back("reset");
        reset = &handler;
    }
    void DeleteHandler(pal4::ScriptWaitHandlerBaseState& handler,
                       unsigned flags) override {
        Check(flags == 1, "deleting-destructor flag");
        events.emplace_back("delete");
        deleted = &handler;
    }
    void* AllocatePointers(std::size_t bytes) override {
        events.emplace_back("allocate");
        allocations.push_back(bytes);
        return new std::byte[bytes];
    }
    void FreePointers(void* pointers) override {
        events.emplace_back("free");
        delete[] static_cast<std::byte*>(pointers);
    }
};

pal4::ScriptWaitHandlerBaseState Handler(int marker, char* text) {
    pal4::ScriptWaitHandlerBaseState handler{};
    handler.marker = marker;
    handler.text_data = reinterpret_cast<std::uint8_t*>(text);
    handler.text_end = handler.text_data + std::char_traits<char>::length(text);
    return handler;
}

std::size_t Count(const pal4::ScriptExecutionContextState& state) {
    if (state.buffer_36 == nullptr) return 0;
    return static_cast<pal4::ScriptWaitHandlerBaseState**>(state.buffer_40) -
           static_cast<pal4::ScriptWaitHandlerBaseState**>(state.buffer_36);
}

std::size_t Capacity(const pal4::ScriptExecutionContextState& state) {
    if (state.buffer_36 == nullptr) return 0;
    return static_cast<pal4::ScriptWaitHandlerBaseState**>(state.buffer_44) -
           static_cast<pal4::ScriptWaitHandlerBaseState**>(state.buffer_36);
}

void TestGrowthAndReturn() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    char one[] = "one";
    char two[] = "two";
    char three[] = "three";
    char four[] = "four";
    auto first = Handler(1, one);
    auto second = Handler(2, two);
    auto third = Handler(3, three);
    auto fourth = Handler(4, four);
    pal4::ScriptWaitHandlerBaseState* handlers[] = {
        &first, &second, &third, &fourth};
    for (unsigned index = 0; index < 4; ++index) {
        const auto result = pal4::ScriptExecutionContext::AddWaitHandler(
            state, *handlers[index], runtime);
        Check(Count(state) == index + 1 &&
                  Capacity(state) ==
                      (index == 0 ? 1u : index == 1 ? 2u : 4u),
              "handler pointer array grows 1, 2, 4 and uses spare slot");
        auto* begin = static_cast<pal4::ScriptWaitHandlerBaseState**>(
            state.buffer_36);
        for (unsigned j = 0; j <= index; ++j) {
            Check(begin[j] == handlers[j], "growth preserves pointer order");
        }
        const auto expected = index == 3 ? static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(state.buffer_40)) : index;
        Check(result == expected,
              "allocation returns old count; spare slot returns new end pointer");
    }
    Check(runtime.allocations == std::vector<std::size_t>{4, 8, 16},
          "pointer array allocation sizes match x86 capacity growth");
    runtime.FreePointers(state.buffer_36);
}

void TestDuplicateReplacementAndMarkerGate() {
    pal4::ScriptExecutionContextState state{};
    Runtime runtime;
    char same[] = "same";
    char other[] = "other";
    auto first = Handler(7, same);
    auto second = Handler(8, other);
    auto different_key = Handler(7, other);
    auto replacement = Handler(7, same);
    pal4::ScriptExecutionContext::AddWaitHandler(state, first, runtime);
    pal4::ScriptExecutionContext::AddWaitHandler(state, second, runtime);

    runtime.events.clear();
    pal4::ScriptExecutionContext::AddWaitHandler(state, different_key, runtime);
    Check(Count(state) == 3 && runtime.reset == nullptr &&
              runtime.events[0] == "compare",
          "same marker with different text does not replace");

    runtime.events.clear();
    const auto result = pal4::ScriptExecutionContext::AddWaitHandler(
        state, replacement, runtime);
    auto* begin = static_cast<pal4::ScriptWaitHandlerBaseState**>(
        state.buffer_36);
    Check(Count(state) == 3 && begin[0] == &second &&
              begin[1] == &different_key && begin[2] == &replacement &&
              runtime.reset == &first && runtime.deleted == &first &&
              runtime.events == std::vector<std::string>{
                                    "compare", "reset", "delete"} &&
              result == static_cast<std::uint32_t>(
                  reinterpret_cast<std::uintptr_t>(state.buffer_40)),
          "matching old handler is destroyed, compacted and replaced");

    auto negative = Handler(-1, same);
    runtime.events.clear();
    pal4::ScriptExecutionContext::AddWaitHandler(state, negative, runtime);
    Check(Count(state) == 4 &&
              runtime.events == std::vector<std::string>{} &&
              begin[3] == &negative,
          "negative incoming marker bypasses deduplication");
    runtime.FreePointers(state.buffer_36);
}

}  // namespace

int main() {
    TestGrowthAndReturn();
    TestDuplicateReplacementAndMarkerGate();
}
