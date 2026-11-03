#include "pal4/file_stream_lifecycle.h"
#include "pal4/crt_atexit_stub.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IFileStreamLifecycleRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::int32_t close_result{};
    std::uint32_t facet_result{0xb000u};
    std::uint32_t clear_state{};
    std::uint32_t clear_address{};

    Runtime() {
        words[0x849fb8u + 4u] = 12u;
        words[0x849fb0u + 4u] = 4u;
    }
    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("word");
    }
    [[nodiscard]] std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        events.emplace_back("byte");
    }
    [[nodiscard]] std::int32_t CloseFile(
        const std::uint32_t handle) override {
        Check(handle == 0x9000u, "closes the filebuf handle");
        events.emplace_back("close");
        return close_result;
    }
    void InitializeFileBuffer(const std::uint32_t buffer,
                              const std::uint32_t source,
                              const std::uint32_t mode) override {
        Check(buffer == 0x100cu && source == 0u && mode == 2u,
              "successful fclose reinitializes filebuf with mode two");
        events.emplace_back("filebuf-init");
    }
    void ResetText(const std::uint32_t text,
                   const std::uint8_t flag) override {
        Check(text == 0xa000u && flag == 1u,
              "text buffer reset uses flag one");
        events.emplace_back("text-reset");
    }
    void Free(const std::uint32_t address) override {
        Check(address == 0xa000u, "text buffer freed after reset");
        events.emplace_back("free");
    }
    void DeleteLocaleRecord(const std::uint32_t record,
                            const std::uint32_t flags) override {
        Check(record == 0xb000u && flags == 1u,
              "zero-reference locale record is deleted");
        events.emplace_back("locale-delete");
    }
    [[nodiscard]] std::uint32_t DecrementFacet(
        const std::uint32_t facet) override {
        Check(facet == 0xc000u, "filebuf locale facet decremented");
        events.emplace_back("facet-decref");
        return facet_result;
    }
    void DeleteFacet(const std::uint32_t facet,
                     const std::uint32_t flags) override {
        Check(facet == facet_result && flags == 1u,
              "released facet deleted with flag one");
        events.emplace_back("facet-delete");
    }
    void InitializeIosBase(const std::uint32_t ios) override {
        Check(ios == 0x100cu, "ios-base initializer receives virtual offset");
        events.emplace_back("ios-init");
    }
    void ClearIosBase(const std::uint32_t ios,
                      const std::uint32_t state,
                      const bool throw_on_error) override {
        Check(!throw_on_error, "ios clear does not throw on error");
        clear_address = ios;
        clear_state = state;
        events.emplace_back("ios-clear");
    }
};

struct ExitApi final : pal4::ICrtAtexitApi {
    std::vector<pal4::CrtExitCallback> callbacks;
    int result{-7};
    [[nodiscard]] int Register(pal4::CrtExitCallback callback) override {
        callbacks.push_back(callback);
        return result;
    }
};

struct ReleaseRuntime final : pal4::IFileStreamReleaseRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> released;
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override { return words[address]; }
    [[nodiscard]] std::uint32_t Free(
        const std::uint32_t address) override {
        released.push_back(address);
        return address + 1u;
    }
};

void TestIostreamInitialization() {
    Runtime runtime;
    Check(pal4::FileStreamLifecycle::InitializeIostream(
              0x1000u, 0x3000u, true, runtime) == 0x1000u,
          "constructor returns complete object address");
    Check(runtime.words[0x1000u] == 0x849fb8u &&
              runtime.words[0x1008u] == 0x849fb0u &&
              runtime.words[0x1030u] == 0u &&
              runtime.words[0x1034u] == 0x3000u &&
              runtime.words[0x1038u] == 0u &&
              runtime.bytes[0x103cu] == 0x20u &&
              runtime.words[0x100cu] == 0x849facu &&
              runtime.events.back() == "word",
          "virtual base, streambuf, fill byte and final vtable are stored");

    Runtime null_buffer;
    null_buffer.words[0x1010u] = 0x120u;
    (void)pal4::FileStreamLifecycle::InitializeIostream(
        0x1000u, 0u, true, null_buffer);
    Check(null_buffer.clear_address == 0x100cu &&
              null_buffer.clear_state == 0x124u,
          "missing streambuf sets ios badbit while retaining other state");

    Runtime existing_base;
    existing_base.words[0x1000u] = 0x849fb8u;
    existing_base.words[0x1008u] = 0x849fb0u;
    existing_base.words[0x1030u] = 0xfeedu;
    (void)pal4::FileStreamLifecycle::InitializeIostream(
        0x1000u, 0x3000u, false, existing_base);
    Check(existing_base.words[0x1030u] == 0xfeedu &&
              existing_base.words[0x1034u] == 0x3000u,
          "non-virtual construction preserves preexisting virtual base field");
}

void TestIostreamBaseDestructor() {
    Runtime runtime;
    runtime.words[0x1000u] = 0x849fb8u;
    runtime.words[0x1008u] = 0x849fb0u;
    Check(pal4::FileStreamLifecycle::DestroyIostreamBase(
              0x100cu, runtime) == 0x849fb8u &&
              runtime.words[0x100cu] == 0x8682c0u,
          "base destructor restores iostream, ostream and istream in order");
}

void TestCompleteDestructor() {
    Runtime runtime;
    runtime.words[0x1000u] = 0x849fb8u;
    runtime.words[0x1008u] = 0x849fb0u;
    runtime.bytes[0x100cu + 72u] = 1u;
    runtime.words[0x100cu + 80u] = 0x9000u;
    runtime.words[0x100cu + 68u] = 0xa000u;
    runtime.words[0x100cu + 76u] = 0xb000u;
    runtime.words[0xb000u + 4u] = 1u;
    runtime.words[0x100cu + 52u] = 0xc000u;

    Check(pal4::FileStreamLifecycle::Destroy(
              0x1060u, runtime) == 0x849fb8u &&
              runtime.words[0xb004u] == 0u &&
              runtime.words[0x100cu] == 0x8682c0u,
          "full destructor closes file, releases records and restores bases");
    std::vector<std::string> actions;
    for (const auto& event : runtime.events)
        if (event != "word" && event != "byte") actions.push_back(event);
    Check(actions == (std::vector<std::string>{
              "close", "filebuf-init", "text-reset", "free",
              "locale-delete", "facet-decref", "facet-delete"}),
          "external cleanup follows native call order");

    Runtime preserved;
    preserved.words[0x1000u] = 0x849fb8u;
    preserved.words[0x1008u] = 0x849fb0u;
    preserved.bytes[0x100cu + 72u] = 1u;
    preserved.words[0x100cu + 80u] = 0x9000u;
    preserved.close_result = -1;
    preserved.words[0x100cu + 76u] = 0xb000u;
    preserved.words[0xb004u] = 0xffffffffu;
    preserved.words[0x100cu + 52u] = 0xc000u;
    preserved.facet_result = 0u;
    (void)pal4::FileStreamLifecycle::Destroy(0x1060u, preserved);
    Check(preserved.words[0xb004u] == 0xffffffffu,
          "saturated locale reference count is not decremented");
    for (const auto& event : preserved.events)
        Check(event != "filebuf-init" && event != "locale-delete" &&
                  event != "facet-delete",
              "failed close and retained references skip optional release");
}

void TestExitRegistrationAndMemberRelease() {
    ExitApi api;
    std::uint8_t first = 0x80u;
    std::uint8_t second = 0x40u;
    Check(pal4::CrtExitRegistration::InitializeFileStreamGuards(
              first, second, api) == -7 &&
              first == 0x81u && second == 0x41u &&
              api.callbacks.size() == 2u &&
              api.callbacks[0] == api.callbacks[1],
          "file-stream initializer sets both guards and registers twice");
    Check(pal4::CrtExitRegistration::InitializeFileStreamGuards(
              first, second, api) == -7 &&
              api.callbacks.size() == 4u &&
              pal4::CrtExitRegistration::RegisterFileStreamGuardOne(api) == -7 &&
              api.callbacks.size() == 5u,
          "already-set guards still register callbacks and result is forwarded");

    ReleaseRuntime release;
    Check(pal4::FileStreamLifecycle::FreeMembers(0u, release) == 0u &&
              release.released.empty(),
          "null file-stream pointer causes no allocator calls");
    release.words[0x1000u] = 0x2000u;
    release.words[0x1004u] = 0x3000u;
    release.words[0x1008u] = 0x4000u;
    Check(pal4::FileStreamLifecycle::FreeMembers(0x1000u, release) ==
              0x1001u &&
              release.released ==
                  (std::vector<std::uint32_t>{
                      0x4000u, 0x3000u, 0x2000u, 0x1000u}),
          "four allocator calls follow the native reverse-member order");
}

}  // namespace

int main() {
    TestIostreamInitialization();
    TestIostreamBaseDestructor();
    TestCompleteDestructor();
    TestExitRegistrationAndMemberRelease();
}
