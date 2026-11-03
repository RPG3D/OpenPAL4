#include "pal4/script_object.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptObjectRuntime {
    std::vector<std::string> calls;
    void InitializeTaskContainer(std::uint32_t address) override {
        calls.push_back("init:" + std::to_string(address));
    }
    std::uint32_t DestroyTaskContainer(std::uint32_t address) override {
        calls.push_back("destroy:" + std::to_string(address));
        return 0x77u;
    }
};

struct EventRuntime final : pal4::IScriptEventNodeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> factory_results;
    std::vector<std::uint32_t> visited_ids;
    std::vector<std::string> calls;

    std::uint32_t NilAddress() override { return 0xdeadu; }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == 0x300cu || address == 0x310cu || address == 0x320cu)
            visited_ids.push_back(address);
        return words.at(address);
    }
    std::uint32_t CallFactory(std::uint32_t function_address) override {
        calls.push_back("factory:" + std::to_string(function_address));
        return factory_results.at(function_address);
    }
    void AddTaskAndInitBase(std::uint32_t owner, std::uint32_t task,
                            std::uint32_t source,
                            std::uint32_t timing) override {
        calls.push_back("add:" + std::to_string(owner) + ":" +
                        std::to_string(task) + ":" +
                        std::to_string(source) + ":" +
                        std::to_string(timing));
    }
};

void SetNode(EventRuntime& runtime, std::uint32_t node,
             std::uint32_t left, std::uint32_t right,
             std::int32_t id, std::uint32_t factory) {
    runtime.words[node] = left;
    runtime.words[node + 8u] = right;
    runtime.words[node + 12u] = static_cast<std::uint32_t>(id);
    runtime.words[node + 16u] = factory;
}

EventRuntime MakeEventTree() {
    EventRuntime runtime;
    runtime.words[0x1008u] = 0x2000u;
    runtime.words[0x2004u] = 0x3000u;
    SetNode(runtime, 0x3000u, 0x3100u, 0x3200u, 10, 0x5010u);
    SetNode(runtime, 0x3100u, runtime.NilAddress(),
            runtime.NilAddress(), -2, 0x5020u);
    SetNode(runtime, 0x3200u, runtime.NilAddress(),
            runtime.NilAddress(), 20, 0x5030u);
    runtime.factory_results = {
        {0x5010u, 0x7010u}, {0x5020u, 0x7020u}, {0x5030u, 0u}};
    return runtime;
}

void TestEventSearch() {
    auto runtime = MakeEventTree();
    runtime.words[0x4000u] = static_cast<std::uint32_t>(-3);
    Check(pal4::ScriptObject::SearchEventNode(0x1004u, 0x4000u,
                                              runtime) == 0x3100u &&
              runtime.visited_ids == std::vector<std::uint32_t>{
                  0x300cu, 0x310cu},
          "signed lower bound traverses left for a negative class ID");
    runtime.visited_ids.clear();
    runtime.words[0x4000u] = 11u;
    Check(pal4::ScriptObject::SearchEventNode(0x1004u, 0x4000u,
                                              runtime) == 0x3200u &&
              runtime.visited_ids == std::vector<std::uint32_t>{
                  0x300cu, 0x320cu},
          "lower bound traverses right then accepts first larger ID");
    runtime.visited_ids.clear();
    runtime.words[0x4000u] = 25u;
    Check(pal4::ScriptObject::SearchEventNode(0x1004u, 0x4000u,
                                              runtime) == 0x2000u,
          "search returns the header when no class ID qualifies");
    runtime.words[0x2004u] = runtime.NilAddress();
    runtime.visited_ids.clear();
    Check(pal4::ScriptObject::SearchEventNode(0x1004u, 0x4000u,
                                              runtime) == 0x2000u &&
              runtime.visited_ids.empty(),
          "empty tree returns header without reading class ID");
}

void TestEventProcessing() {
    auto runtime = MakeEventTree();
    runtime.words[0x4000u] = 10u;
    Check(pal4::ScriptObject::ProcessEventData(
              0x9000u, 0x4000u, 0x1000u, 0xa000u, 35u,
              runtime) == 0x7010u &&
              runtime.calls == std::vector<std::string>{
                  "factory:20496", "add:36864:28688:40960:35"},
          "exact class ID creates a task then submits it with source and timing");
    runtime.calls.clear();
    runtime.words[0x4000u] = 11u;
    Check(pal4::ScriptObject::ProcessEventData(
              0x9000u, 0x4000u, 0x1000u, 0xa000u, 36u,
              runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "add:36864:0:40960:36"},
          "larger lower-bound candidate is rejected but null task is submitted");
    runtime.calls.clear();
    runtime.words[0x4000u] = 25u;
    Check(pal4::ScriptObject::ProcessEventData(
              0x9000u, 0x4000u, 0x1000u, 0xa000u, 37u,
              runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "add:36864:0:40960:37"},
          "header result submits a null task without factory call");
    runtime.calls.clear();
    runtime.words[0x4000u] = 20u;
    Check(pal4::ScriptObject::ProcessEventData(
              0x9000u, 0x4000u, 0x1000u, 0xa000u, 38u,
              runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "factory:20528", "add:36864:0:40960:38"},
          "factory may return null and add still runs");
    runtime.calls.clear();
    runtime.words[0x4000u] = static_cast<std::uint32_t>(-2);
    Check(pal4::ScriptObject::ProcessEventData(
              0x9000u, 0x4000u, 0x1000u, 0xa000u, 39u,
              runtime) == 0x7020u &&
              runtime.calls == std::vector<std::string>{
                  "factory:20512", "add:36864:28704:40960:39"},
          "negative exact ID uses signed comparison and the left-node factory");
}

void TestResolvedFactories(){
    auto runtime=MakeEventTree();runtime.words.clear();
    Check(pal4::ScriptObject::ProcessResolvedFactory(0x9000u,0x5010u,0xa000u,40u,runtime)==0x7010u && runtime.calls==std::vector<std::string>{"factory:20496","add:36864:28688:40960:40"},"resolved factory uses the same call and submission order without tree reads");
    runtime.calls.clear();
    Check(pal4::ScriptObject::ProcessResolvedFactory(0x9000u,std::nullopt,0xa000u,41u,runtime)==0u && runtime.calls==std::vector<std::string>{"add:36864:0:40960:41"},"missing resolved factory still submits the null task");
    runtime.calls.clear();runtime.factory_results[0]=0;
    Check(pal4::ScriptObject::ProcessResolvedFactory(0x9000u,std::optional<std::uint32_t>{0},0xa000u,42u,runtime)==0u && runtime.calls==std::vector<std::string>{"factory:0","add:36864:0:40960:42"},"a present zero address retains the original factory invocation");
}

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::ScriptObject::Initialize(0x1000u, runtime) == 0x1000u &&
              runtime.calls == std::vector<std::string>{"init:4096"},
          "script initializer forwards this and returns the object address");
    Check(pal4::ScriptObject::Destroy(0x1000u, runtime) == 0x77u &&
              runtime.calls == std::vector<std::string>{
                  "init:4096", "destroy:4096"},
          "script destroy thunk forwards this and callee result");
    pal4::ScriptObjectRuntimeStub stub;
    Check(pal4::ScriptObject::Initialize(0x1000u, stub) == 0x1000u &&
              pal4::ScriptObject::Destroy(0x1000u, stub) == 0u,
          "script object runtime stub links");
    TestEventSearch();
    TestEventProcessing();
    TestResolvedFactories();
    pal4::ScriptEventNodeRuntimeStub event_stub;
    Check(pal4::ScriptObject::ProcessEventData(
              0x9000u, 0x4000u, 0x1000u, 0xa000u, 39u,
              event_stub) == 0u,
          "script event runtime stub links");
    std::cout << "script object legacy search and resolved factory contracts passed\n";
}
