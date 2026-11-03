#include "pal4/effect_system_task_creator.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemTaskCreateRuntime {
    std::optional<std::string> name = "SlowMotionTask";
    std::uint32_t factory = 0x6412e0u;
    std::uint32_t task = 0x5000u;
    bool initialized = true;
    std::vector<std::string> calls;

    std::optional<std::string> StringAttribute(
        std::uint32_t document, std::string_view key) override {
        Check(document == 0x1000u && key == "Attr_Class_Name",
              "task creator reads exact XML attribute");
        calls.emplace_back("attribute");
        return name;
    }
    std::uint32_t FindFactory(std::string_view query) override {
        Check(query == "SlowMotionTask", "task creator searches class name");
        calls.emplace_back("find");
        return factory;
    }
    std::uint32_t InvokeFactory(std::uint32_t address) override {
        Check(address == factory, "task creator invokes found factory");
        calls.emplace_back("create");
        return task;
    }
    bool InitializeTask(std::uint32_t object,
                        std::uint32_t document) override {
        Check(object == task && document == 0x1000u,
              "task initialization gets created object and original XML");
        calls.emplace_back("initialize");
        return initialized;
    }
    void DeleteTask(std::uint32_t object, std::uint32_t flags) override {
        Check(object == task && flags == 1u,
              "failed task initialization invokes deleting destructor");
        calls.emplace_back("delete");
    }
};

void TestGuards() {
    Runtime runtime;
    Check(pal4::EffectSystemTaskCreator::CreateFromDocument(0u, runtime) == 0u &&
              runtime.calls.empty(),
          "null XML returns without side effects");
    runtime.name.reset();
    Check(pal4::EffectSystemTaskCreator::CreateFromDocument(
              0x1000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"attribute"},
          "missing class attribute stops before lookup");
    runtime = Runtime{};
    runtime.factory = 0u;
    Check(pal4::EffectSystemTaskCreator::CreateFromDocument(
              0x1000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"attribute", "find"},
          "missing factory stops before construction");
    runtime = Runtime{};
    runtime.task = 0u;
    Check(pal4::EffectSystemTaskCreator::CreateFromDocument(
              0x1000u, runtime) == 0u &&
              runtime.calls ==
                  std::vector<std::string>{"attribute", "find", "create"},
          "failed factory stops before initialization");
}

void TestInitialization() {
    Runtime runtime;
    Check(pal4::EffectSystemTaskCreator::CreateFromDocument(
              0x1000u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "attribute", "find", "create", "initialize"},
          "successful initialization retains task");
    runtime = Runtime{};
    runtime.initialized = false;
    Check(pal4::EffectSystemTaskCreator::CreateFromDocument(
              0x1000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "attribute", "find", "create", "initialize", "delete"},
          "failed initialization deletes task and returns null");
}

}  // namespace

int main() {
    TestGuards();
    TestInitialization();
}
