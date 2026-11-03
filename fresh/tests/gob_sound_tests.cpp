#include "pal4/gob_sound.h"
#include "pal4/standard_stream_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGobSoundRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = object;
    std::uint32_t stop_result = 0x55u;
    bool fail_base = false;
    bool fail_name = false;
    std::uint32_t visible = 0u;
    std::uint8_t visibility_result = 0x73u;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobSound::kSize, "sound allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == object, "sound free address");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == object, "sound base construct address");
        calls.emplace_back("construct base");
        if (fail_base) throw std::runtime_error("base failed");
    }
    void DestructBase(std::uint32_t address) override {
        Check(address == object, "sound base destruct address");
        calls.emplace_back("destruct base");
    }
    void InitializeNameRecord(std::uint32_t record) override {
        Check(record == object + 0x1e4u, "sound name record address");
        calls.emplace_back("initialize name");
        if (fail_name) throw std::runtime_error("name failed");
    }
    void ReleaseNameRecord(std::uint32_t record) override {
        Check(record == object + 0x1e4u, "sound name release address");
        calls.emplace_back("release name");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address - object));
    }
    void CleanupBaseResources(std::uint32_t address) override {
        Check(address == object, "sound base cleanup address");
        calls.emplace_back("cleanup base");
    }
    std::uint32_t StopAndRemoveAudioHandle(std::uint32_t handle) override {
        Check(handle == object + 0x1e0u, "audio handle address");
        calls.emplace_back("stop/remove handle");
        return stop_result;
    }
    std::uint8_t SetBaseVisible(std::uint32_t address,
                                std::uint32_t value) override {
        Check(address == object, "sound visibility object");
        visible = value;
        calls.emplace_back("base visible");
        return visibility_result;
    }
};

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::GobSound::Create(runtime) == Runtime::object &&
          runtime.words[Runtime::object] == pal4::GobSound::kVtable &&
          runtime.words[Runtime::object + 0x1e0u] == 0u &&
          runtime.words[Runtime::object + 0x170u] == 0u &&
          runtime.words[Runtime::object + 0x1dcu] == 0u &&
          runtime.words[Runtime::object + 0x1f8u] == 0x3f800000u &&
          runtime.calls == (std::vector<std::string>{
              "allocate", "construct base", "write 480", "initialize name",
              "write 488", "write 492", "write 496", "write 368",
              "write 500", "write 508", "write 512", "write 516",
              "write 476", "write 0", "write 504"}),
          "sound constructor layout and write order");
    pal4::GobSound::NoOp(0x77u);
    Check(pal4::GobSound::ReturnZero() == 0u &&
          pal4::GobSound::TypeKey() == 3u &&
          pal4::StandardStreamLifecycle::ConstructIostreamInit(0x3456u) ==
              0x3456u && runtime.calls.size() == 15u,
          "no-op, zero, type thunk and iostream identity");
    runtime.calls.clear();
    pal4::GobSound::Destruct(Runtime::object, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "write 0", "release name", "write 488", "write 492",
              "write 496", "destruct base"}),
          "sound destructor name release and base order");
    Runtime null;
    null.allocation = 0u;
    Check(pal4::GobSound::Create(null) == 0u &&
          null.calls == (std::vector<std::string>{"allocate"}),
          "sound null allocation");
    Runtime failing;
    failing.fail_base = true;
    try {
        (void)pal4::GobSound::Create(failing);
        throw std::runtime_error("missing base exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "base failed" &&
              failing.calls == (std::vector<std::string>{
                  "allocate", "construct base", "free"}),
              "base constructor exception cleanup");
    }
    Runtime name_failure;
    name_failure.fail_name = true;
    try {
        (void)pal4::GobSound::Create(name_failure);
        throw std::runtime_error("missing name exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "name failed" &&
              name_failure.calls == (std::vector<std::string>{
                  "allocate", "construct base", "write 480",
                  "initialize name", "free"}),
              "name constructor exception cleanup");
    }
}

void CheckCleanup() {
    Runtime runtime;
    Check(pal4::GobSound::CleanupResources(
              Runtime::object, runtime) == 0x55u &&
          runtime.words[Runtime::object + 0x1f8u] == 0x3f800000u &&
          runtime.words[Runtime::object + 0x1f4u] == 0u &&
          runtime.words[Runtime::object + 0x1fcu] == 0u &&
          runtime.words[Runtime::object + 0x200u] == 0u &&
          runtime.words[Runtime::object + 0x204u] == 0u &&
          runtime.calls == (std::vector<std::string>{
              "write 500", "write 508", "write 512", "write 516",
              "write 504", "cleanup base", "stop/remove handle"}),
          "sound cleanup resets timer before base cleanup and handle call");
    runtime.stop_result = 0u;
    runtime.calls.clear();
    Check(pal4::GobSound::CleanupResources(
              Runtime::object, runtime) == 0u &&
          runtime.calls.back() == "stop/remove handle",
          "audio handle result forwarded");
}

void CheckVisibilityThunk() {
    Runtime runtime;
    Check(pal4::GobSound::SetVisible(Runtime::object, 0x123u, runtime) ==
              0x73u && runtime.visible == 0x123u &&
          runtime.calls == (std::vector<std::string>{"base visible"}),
          "visibility thunk forwards argument and return");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckCleanup();
        CheckVisibilityThunk();
        std::cout << "gob_sound_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
