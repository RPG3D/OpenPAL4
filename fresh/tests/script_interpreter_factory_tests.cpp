#include "pal4/script_interpreter_factory.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptInterpreterFactoryRuntime {
    std::vector<std::string> events;
    bool fail_allocate = false;
    bool throw_construct = false;
    bool initialize_result = true;
    std::uint32_t seen_context = 0;
    std::uint32_t seen_callback = 0;
    static constexpr int base_marker = 1;

    void* Allocate(std::size_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        if (fail_allocate) {
            fail_allocate = false;
            return nullptr;
        }
        return std::malloc(bytes);
    }

    void Free(void* pointer) override {
        Check(pointer != nullptr, "free receives nonnull pointer");
        events.push_back("free");
        std::free(pointer);
    }

    std::size_t Length(const char* text) override {
        events.push_back("length");
        return std::strlen(text);
    }

    void Copy(char* target, const char* text) override {
        Check(target != nullptr, "text allocation succeeds");
        events.push_back("copy");
        std::strcpy(target, text);
    }

    pal4::ScriptInterpreterBaseState* ConstructText(void* memory) override {
        events.push_back("construct:text");
        if (throw_construct) {
            throw std::runtime_error("constructor");
        }
        auto* state = ::new (memory) pal4::ScriptInterpreterBaseState{};
        return pal4::ScriptInterpreterBase::Initialize(*state, &base_marker);
    }

    pal4::ScriptInterpreterBaseState* ConstructBinary(void* memory) override {
        events.push_back("construct:binary");
        if (throw_construct) {
            throw std::runtime_error("constructor");
        }
        auto* state = ::new (memory) pal4::ScriptInterpreterBaseState{};
        return pal4::ScriptInterpreterBase::Initialize(*state, &base_marker);
    }

    bool Initialize(pal4::ScriptInterpreterBaseState* interpreter,
                    std::uint32_t context, std::uint32_t callback) override {
        events.push_back("initialize");
        Check(interpreter->path && interpreter->full_path && interpreter->name,
              "three names precede virtual initialization");
        seen_context = context;
        seen_callback = callback;
        return initialize_result;
    }

    void Destroy(pal4::ScriptInterpreterBaseState* interpreter,
                 unsigned flags) override {
        events.push_back("destroy:" + std::to_string(flags));
        pal4::ScriptInterpreterBase::DestroyAt(interpreter, flags,
                                               &base_marker, *this);
    }
};

void CheckEvents(const Runtime& runtime,
                 std::initializer_list<const char*> expected,
                 const char* message) {
    std::vector<std::string> wanted(expected.begin(), expected.end());
    Check(runtime.events == wanted, message);
}

}  // namespace

int main() {
    Runtime runtime;
    using pal4::ScriptInterpreterFactory;

    Check(ScriptInterpreterFactory::Create(-1, "dir", "dir/a.cs", "a",
                                           1, 2, runtime) == nullptr &&
              ScriptInterpreterFactory::Create(2, "dir", "dir/a.cs", "a",
                                               1, 2, runtime) == nullptr &&
              runtime.events.empty(),
          "unsupported modes return without allocating");

    auto* text = ScriptInterpreterFactory::Create(
        0, "dir", "dir/a.cs", "a", 0x12345678u, 0xfedcba98u, runtime);
    Check(text && std::strcmp(text->path, "dir") == 0 &&
              std::strcmp(text->full_path, "dir/a.cs") == 0 &&
              std::strcmp(text->name, "a") == 0 &&
              runtime.seen_context == 0x12345678u &&
              runtime.seen_callback == 0xfedcba98u,
          "text mode constructs and initializes with original arguments");
    CheckEvents(runtime,
                {"allocate:28", "construct:text", "length", "allocate:4", "copy",
                 "length", "allocate:9", "copy", "length", "allocate:2", "copy",
                 "initialize"},
                "text mode preserves allocation and setter order");
    Check(ScriptInterpreterFactory::Release(text, runtime) == text,
          "release returns the original interpreter address");
    runtime.events.clear();

    Check(ScriptInterpreterFactory::Release(nullptr, runtime) == nullptr &&
              runtime.events.empty(),
          "null release skips virtual destruction");

    auto* binary = ScriptInterpreterFactory::Create(
        1, "x", "x/b.csb", "b", 3, 4, runtime);
    Check(binary && runtime.seen_context == 3 && runtime.seen_callback == 4,
          "binary mode forwards context and callback");
    Check(runtime.events.size() == 12 && runtime.events[0] == "allocate:40" &&
              runtime.events[1] == "construct:binary" &&
              runtime.events.back() == "initialize",
          "binary mode uses 40-byte constructor and same setup sequence");
    Check(ScriptInterpreterFactory::Release(binary, runtime) == binary,
          "binary release uses the same deleting destructor slot");
    runtime.events.clear();

    Check(!ScriptInterpreterFactory::Create(0, nullptr, "full", "name",
                                            1, 2, runtime),
          "missing path aborts creation");
    CheckEvents(runtime, {"allocate:28", "construct:text", "destroy:1", "free"},
                "missing path destroys without later setters");
    runtime.events.clear();

    Check(!ScriptInterpreterFactory::Create(0, "path", nullptr, "name",
                                            1, 2, runtime),
          "missing full path aborts creation");
    CheckEvents(runtime,
                {"allocate:28", "construct:text", "length", "allocate:5", "copy",
                 "destroy:1", "free", "free"},
                "missing full path releases path then object");
    runtime.events.clear();

    Check(!ScriptInterpreterFactory::Create(1, "path", "full", nullptr,
                                            1, 2, runtime),
          "missing name aborts creation");
    CheckEvents(runtime,
                {"allocate:40", "construct:binary", "length", "allocate:5", "copy",
                 "length", "allocate:5", "copy", "destroy:1", "free", "free",
                 "free"},
                "missing name releases both strings then object");
    runtime.events.clear();

    runtime.initialize_result = false;
    Check(!ScriptInterpreterFactory::Create(0, "p", "f", "n", 7, 8, runtime),
          "virtual initialization failure aborts creation");
    Check(runtime.events.size() == 17 &&
              runtime.events[runtime.events.size() - 5] == "destroy:1" &&
              runtime.events.back() == "free",
          "initialization failure calls deleting destructor after all setters");
    runtime.events.clear();
    runtime.initialize_result = true;

    runtime.throw_construct = true;
    bool propagated = false;
    try {
        (void)ScriptInterpreterFactory::Create(1, "p", "f", "n", 0, 0, runtime);
    } catch (const std::runtime_error&) {
        propagated = true;
    }
    Check(propagated, "constructor exception propagates");
    CheckEvents(runtime, {"allocate:40", "construct:binary", "free"},
                "constructor exception frees raw allocation only");
    runtime.events.clear();
    runtime.throw_construct = false;

    runtime.fail_allocate = true;
    Check(!ScriptInterpreterFactory::Create(0, nullptr, "f", "n", 0, 0, runtime),
          "allocation failure returns null for null path");
    CheckEvents(runtime, {"allocate:28"},
                "allocation failure does not construct an interpreter");
}
