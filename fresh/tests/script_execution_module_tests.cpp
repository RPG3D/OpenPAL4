#include "pal4/script_execution_context.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Boundary final : pal4::IScriptModuleBoundary {
    std::vector<std::string> events;
    std::string name = "entry";
    int kind = 0;
    int add_result = 0;
    int build_result = 0;
    int load_result = 0;
    std::uintptr_t raw_message = 0;
    std::string raised;
    std::string logged;
    std::uint32_t seen_logger = 0;
    void* expected_interpreter = reinterpret_cast<void*>(0x1234u);
    void* expected_engine = reinterpret_cast<void*>(0x5678u);
    std::byte data[2] = {std::byte{0x43}, std::byte{0x53}};

    void ReportRaw(std::uint32_t logger, std::uintptr_t message) override {
        events.emplace_back("report");
        seen_logger = logger;
        raw_message = message;
    }
    void RaiseError(const char* message) override {
        events.emplace_back("raise");
        raised = message;
        throw std::runtime_error(raised);
    }
    const char* Name(void* interpreter) override {
        Check(interpreter == expected_interpreter, "name interpreter");
        events.emplace_back("name");
        return name.c_str();
    }
    const char* FullPath(void* interpreter) override {
        Check(interpreter == expected_interpreter, "full path interpreter");
        events.emplace_back("full_path");
        return "scene/entry.cs";
    }
    const char* SectionPath(void* interpreter) override {
        Check(interpreter == expected_interpreter, "section interpreter");
        events.emplace_back("section_path");
        return "entry.cs";
    }
    int Kind(void* interpreter) override {
        Check(interpreter == expected_interpreter, "kind interpreter");
        events.emplace_back("kind");
        return kind;
    }
    std::span<const std::byte> Bytecode(void* interpreter) override {
        Check(interpreter == expected_interpreter, "bytecode interpreter");
        events.emplace_back("bytecode");
        return data;
    }
    std::string_view Source(void* interpreter) override {
        Check(interpreter == expected_interpreter, "source interpreter");
        events.emplace_back("source");
        return "void main() {}";
    }
    void EmitLog(std::uint32_t logger, std::string_view text) override {
        Check(logger == 0x42u, "log sink");
        events.emplace_back("log");
        logged = text;
    }
    int LoadBytecode(void* engine, const char* path,
                     std::span<const std::byte> bytes) override {
        Check(engine == expected_engine &&
                  std::string(path) == "scene/entry.cs" &&
                  bytes.size() == 2 && bytes[0] == std::byte{0x43} &&
                  bytes[1] == std::byte{0x53},
              "bytecode API arguments");
        events.emplace_back("load");
        return load_result;
    }
    int AddSection(void* engine, const char* path, const char* section,
                   std::string_view source) override {
        Check(engine == expected_engine &&
                  std::string(path) == "scene/entry.cs" &&
                  std::string(section) == "entry.cs" &&
                  source == "void main() {}", "section API arguments");
        events.emplace_back("add");
        return add_result;
    }
    int Build(void* engine, const char* path,
              std::uint32_t logger) override {
        Check(engine == expected_engine &&
                  std::string(path) == "scene/entry.cs" &&
                  logger == seen_logger, "build API arguments");
        events.emplace_back("build");
        return build_result;
    }
    void CheckResult(int result, const char* operation,
                     std::uint32_t logger) override {
        Check(logger == seen_logger, "error sink forwarded");
        Check((std::string(operation).find("load byte module") !=
                   std::string::npos && result == load_result) ||
                  (std::string(operation).find("add script section") !=
                       std::string::npos && result == add_result) ||
                  (std::string(operation).find("build script module") !=
                       std::string::npos && result == build_result),
              "operation and result forwarded");
        events.emplace_back("check");
    }
};

void TestInvalidInputs() {
    pal4::ScriptExecutionContextState state{};
    Boundary boundary;
    state.word_8 = 0x42u;
    Check(!pal4::ScriptExecutionContext::AddScriptModule(
              state, nullptr, boundary) &&
              boundary.events == std::vector<std::string>{"report"} &&
              std::string(reinterpret_cast<const char*>(boundary.raw_message)) ==
                  "error<executor::addScriptModule>: add a null script-module\r\n",
          "missing module reports and returns false");

    boundary.events.clear();
    Check(!pal4::ScriptExecutionContext::AddScriptModule(
              state, boundary.expected_interpreter, boundary) &&
              boundary.events == std::vector<std::string>{"report"} &&
              std::string(reinterpret_cast<const char*>(boundary.raw_message)) ==
                  "error<executor::addScriptModule>: null scriptEngine\r\n",
          "missing engine reports before inspecting interpreter");

    state.word_8 = 0;
    try {
        pal4::ScriptExecutionContext::AddScriptModule(state, nullptr,
                                                       boundary);
        Check(false, "missing module must raise without sink");
    } catch (const std::runtime_error&) {
        Check(boundary.raised.find("null script-module") != std::string::npos,
              "missing module exception text");
    }
}

void TestKindsAndEmptyName() {
    pal4::ScriptExecutionContextState state{};
    Boundary boundary;
    state.script_engine = boundary.expected_engine;
    state.word_8 = 0x42u;
    boundary.seen_logger = 0x42u;
    boundary.kind = 2;
    Check(!pal4::ScriptExecutionContext::AddScriptModule(
              state, boundary.expected_interpreter, boundary) &&
              boundary.events == std::vector<std::string>{"name", "kind"},
          "unsupported interpreter kind returns false");

    boundary.name.clear();
    boundary.events.clear();
    Check(!pal4::ScriptExecutionContext::AddScriptModule(
              state, boundary.expected_interpreter, boundary) &&
              boundary.raw_message == 0x90909090u &&
              boundary.events == std::vector<std::string>{
                                     "name", "report", "kind"},
          "empty name forwards raw sentinel then continues");

    state.word_8 = 0;
    boundary.events.clear();
    try {
        pal4::ScriptExecutionContext::AddScriptModule(
            state, boundary.expected_interpreter, boundary);
        Check(false, "empty name must raise without sink");
    } catch (const std::runtime_error&) {
        Check(boundary.raised.find("null scriptFunc") != std::string::npos &&
                  boundary.events == std::vector<std::string>{"name", "raise"},
              "empty-name exception precedes kind query");
    }
}

void TestModulePaths() {
    pal4::ScriptExecutionContextState state{};
    Boundary boundary;
    state.script_engine = boundary.expected_engine;
    state.word_8 = 0x42u;
    boundary.seen_logger = 0x42u;

    boundary.kind = 1;
    boundary.load_result = -7;
    Check(pal4::ScriptExecutionContext::AddScriptModule(
              state, boundary.expected_interpreter, boundary) &&
              boundary.events == std::vector<std::string>{
                                     "name", "kind", "full_path", "log",
                                     "bytecode", "full_path", "load", "check"} &&
              boundary.logged == "\r\n\r\n--<<<< scene/entry.cs >>>>--\r\n",
          "bytecode path forwards data, logs and returns true after check");

    boundary.events.clear();
    boundary.kind = 0;
    boundary.build_result = -1;
    Check(!pal4::ScriptExecutionContext::AddScriptModule(
              state, boundary.expected_interpreter, boundary) &&
              boundary.events == std::vector<std::string>{
                                     "name", "kind", "full_path", "log",
                                     "source", "section_path", "full_path",
                                     "add", "check", "full_path", "build", "check"},
          "source path checks add and build, then returns false for -1");

    boundary.events.clear();
    boundary.build_result = -3;
    Check(pal4::ScriptExecutionContext::AddScriptModule(
              state, boundary.expected_interpreter, boundary),
          "source path returns true for every build status except -1");

    state.word_8 = 0;
    boundary.seen_logger = 0;
    boundary.build_result = 0;
    boundary.events.clear();
    Check(pal4::ScriptExecutionContext::AddScriptModule(
              state, boundary.expected_interpreter, boundary) &&
              boundary.events == std::vector<std::string>{
                                     "name", "kind", "full_path", "source",
                                     "section_path", "full_path", "add", "check",
                                     "full_path", "build", "check"},
          "source loading without a log sink still builds the module");
}

}  // namespace

int main() {
    TestInvalidInputs();
    TestKindsAndEmptyName();
    TestModulePaths();
}
