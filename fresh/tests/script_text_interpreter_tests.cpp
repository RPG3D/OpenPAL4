#include "pal4/script_text_interpreter.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <io.h>
#include <string>
#include <vector>
#include <process.h>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptTextInterpreterRuntime {
    std::vector<std::string> events;
    bool open_succeeds{true};
    std::int32_t file_length{4};
    std::size_t read_result{1};

    void* Allocate(std::size_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return new char[bytes == 0u ? 1u : bytes];
    }
    void Free(void* pointer) override {
        events.push_back("free");
        delete[] static_cast<char*>(pointer);
    }
    std::size_t Length(const char* text) override { return std::strlen(text); }
    void Copy(char* destination, const char* source) override {
        std::strcpy(destination, source);
    }
    void CopyCount(char* destination, const char* source,
                   std::size_t count) override {
        events.push_back("copy:" + std::to_string(count));
        std::strncpy(destination, source, count);
    }
    void* OpenBinary(const char* path) override {
        events.push_back("open:" + std::string(path) + ":rb");
        return open_succeeds ? this : nullptr;
    }
    std::int32_t FileLength(void* stream) override {
        Check(stream == this, "length stream identity");
        events.push_back("length");
        return file_length;
    }
    std::size_t Read(void* destination, std::size_t element_size,
                     std::size_t element_count, void* stream) override {
        Check(stream == this && element_count == 1u,
              "read stream and element count");
        events.push_back("read:" + std::to_string(element_size));
        if (element_size >= 4u) {
            std::memcpy(destination, "CSB!", 4u);
        }
        return read_result;
    }
    void Close(void* stream) override {
        Check(stream == this, "close stream identity");
        events.push_back("close");
    }
};

struct HostFileRuntime final : pal4::IScriptTextInterpreterRuntime {
    void* Allocate(std::size_t bytes) override {
        return new char[bytes == 0u ? 1u : bytes];
    }
    void Free(void* pointer) override {
        delete[] static_cast<char*>(pointer);
    }
    std::size_t Length(const char* text) override { return std::strlen(text); }
    void Copy(char* destination, const char* source) override {
        std::strcpy(destination, source);
    }
    void CopyCount(char* destination, const char* source,
                   std::size_t count) override {
        std::strncpy(destination, source, count);
    }
    void* OpenBinary(const char* path) override {
        return std::fopen(path, "rb");
    }
    std::int32_t FileLength(void* stream) override {
        return static_cast<std::int32_t>(
            _filelength(_fileno(static_cast<std::FILE*>(stream))));
    }
    std::size_t Read(void* destination, std::size_t element_size,
                     std::size_t element_count, void* stream) override {
        return std::fread(destination, element_size, element_count,
                          static_cast<std::FILE*>(stream));
    }
    void Close(void* stream) override {
        std::fclose(static_cast<std::FILE*>(stream));
    }
};

void TestConstructAndLoad() {
    pal4::ScriptTextInterpreterState state;
    Runtime runtime;
    const auto* base_vtable = reinterpret_cast<const void*>(0x1000u);
    const auto* text_vtable = reinterpret_cast<const void*>(0x2000u);
    state.base.full_path = reinterpret_cast<char*>(0x3333u);
    state.source = reinterpret_cast<char*>(0x4444u);
    state.source_size = 99u;
    Check(pal4::ScriptTextInterpreter::Initialize(state, base_vtable,
                                                   text_vtable) == &state &&
              state.base.vtable == text_vtable &&
              state.base.full_path == nullptr && state.base.path == nullptr &&
              state.base.name == nullptr && state.base.extra == nullptr &&
              state.source == nullptr && state.source_size == 0u,
          "text constructor initializes base, derived vtable and source fields");
    Check(!pal4::ScriptTextInterpreter::LoadSource(state, nullptr, 4u, runtime) &&
              state.source == nullptr && state.source_size == 0u &&
              runtime.events.empty(),
          "null source returns false without mutation");
    Check(pal4::ScriptTextInterpreter::LoadSource(state, "abc", 6u, runtime) &&
              state.source_size == 6u &&
              std::memcmp(state.source, "abc\0\0\0", 6u) == 0 &&
              runtime.events == std::vector<std::string>{"allocate:6", "copy:6"},
          "source load allocates exact count and pads with strncpy");
    runtime.events.clear();
    Check(pal4::ScriptTextInterpreter::LoadSource(state, "abcdef", 3u, runtime) &&
              state.source_size == 3u &&
              std::memcmp(state.source, "abc", 3u) == 0 &&
              runtime.events == std::vector<std::string>{
                  "free", "allocate:3", "copy:3"},
          "replacement frees old source and does not invent terminator");
    runtime.events.clear();
    pal4::ScriptTextInterpreter::ReleaseFields(state, base_vtable,
                                                text_vtable, runtime);
    Check(state.source == nullptr && state.source_size == 0u &&
              state.base.vtable == base_vtable &&
              runtime.events == std::vector<std::string>{"free"},
          "derived source is freed before base release");
    runtime.events.clear();
    pal4::ScriptTextInterpreter::ReleaseFields(state, base_vtable,
                                                text_vtable, runtime);
    Check(runtime.events.empty(), "release of empty source does not free");
}

void TestFileLoad() {
    pal4::ScriptTextInterpreterState state;
    Runtime runtime;
    pal4::ScriptTextInterpreter::Initialize(
        state, reinterpret_cast<const void*>(0x1000u),
        reinterpret_cast<const void*>(0x2000u));
    Check(!pal4::ScriptTextInterpreter::LoadFile(state, nullptr, 7u, runtime) &&
              runtime.events.empty(),
          "null path does not touch stream or state");
    state.source = static_cast<char*>(runtime.Allocate(2u));
    state.source_size = 2u;
    runtime.events.clear();
    runtime.open_succeeds = false;
    Check(!pal4::ScriptTextInterpreter::LoadFile(state, "missing.cs", 7u,
                                                  runtime) &&
              state.source_size == 2u && state.source != nullptr &&
              runtime.events == std::vector<std::string>{"open:missing.cs:rb"},
          "open failure leaves old source untouched");
    runtime.events.clear();
    runtime.open_succeeds = true;
    runtime.read_result = 0u;
    Check(pal4::ScriptTextInterpreter::LoadFile(state, "script.cs", 7u,
                                                 runtime) &&
              state.source_size == 4u &&
              std::memcmp(state.source, "CSB!", 4u) == 0 &&
              runtime.events == std::vector<std::string>{
                  "open:script.cs:rb", "free", "length", "allocate:4",
                  "read:4", "close"},
          "successful open replaces buffer and ignores short-read result");
    pal4::ScriptTextInterpreter::ReleaseFields(
        state, reinterpret_cast<const void*>(0x1000u),
        reinterpret_cast<const void*>(0x2000u), runtime);
}

void TestHostCrtFileBoundary() {
    const auto path = std::filesystem::temp_directory_path() /
                      ("pal4_text_cs_" + std::to_string(_getpid()) + ".cs");
    const auto filename = path.string();
    std::FILE* output = std::fopen(filename.c_str(), "wb");
    Check(output != nullptr, "create temporary CS file");
    Check(std::fwrite("abc\0d", 1u, 5u, output) == 5u,
          "write exact binary CS payload");
    std::fclose(output);

    pal4::ScriptTextInterpreterState state;
    HostFileRuntime runtime;
    pal4::ScriptTextInterpreter::Initialize(
        state, reinterpret_cast<const void*>(0x1000u),
        reinterpret_cast<const void*>(0x2000u));
    Check(pal4::ScriptTextInterpreter::LoadFile(state, filename.c_str(), 0u,
                                                 runtime) &&
              state.source_size == 5u &&
              std::memcmp(state.source, "abc\0d", 5u) == 0,
          "host CRT open/filelength/read/close loads exact bytes");
    pal4::ScriptTextInterpreter::ReleaseFields(
        state, reinterpret_cast<const void*>(0x1000u),
        reinterpret_cast<const void*>(0x2000u), runtime);
    std::filesystem::remove(path);
}

void TestSourceQueries() {
    pal4::ScriptTextInterpreterState state;
    Runtime runtime;
    pal4::ScriptTextInterpreter::Initialize(
        state, reinterpret_cast<const void*>(0x1000u),
        reinterpret_cast<const void*>(0x2000u));
    const char empty[] = "";
    state.source_size = 9u;
    Check(pal4::ScriptTextInterpreter::GetSourceOrEmpty(state, empty) == empty &&
              pal4::ScriptTextInterpreter::GetSourceSizeOrZero(state) == 0u,
          "empty source returns original global fallback and zero size");
    state.source = static_cast<char*>(runtime.Allocate(3u));
    std::memcpy(state.source, "abc", 3u);
    state.source_size = 3u;
    Check(pal4::ScriptTextInterpreter::GetSourceOrEmpty(state, empty) ==
              state.source &&
              pal4::ScriptTextInterpreter::GetSourceSizeOrZero(state) == 3u,
          "loaded source pointer and length are returned unchanged");
    pal4::ScriptTextInterpreter::ReleaseFields(
        state, reinterpret_cast<const void*>(0x1000u),
        reinterpret_cast<const void*>(0x2000u), runtime);
}

}  // namespace

int main() {
    TestConstructAndLoad();
    TestFileLoad();
    TestHostCrtFileBoundary();
    TestSourceQueries();
}
