#include "pal4/system_setting_frame.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingFrameRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::uint32_t allocation = 0x4000u;
    std::uint32_t event_name = 0u;
    std::uint32_t event_create = 0u;
    std::string throw_at;

    void MaybeThrow(const char* point) {
        if (throw_at == point) throw std::runtime_error(point);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("byte");
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::SystemSettingFrame::kSize,
              "SystemSetting frame allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    std::uint32_t ProcessEvent(std::uint32_t manager,
                               std::uint32_t name,
                               std::uint32_t create_function) override {
        Check(manager == 0x6000u, "factory manager");
        calls.emplace_back("process");
        event_name = name;
        event_create = create_function;
        return 0x7000u;
    }
    void LoadLayout(std::uint32_t frame, std::uint32_t name,
                    std::uint32_t first_flag,
                    std::uint32_t second_flag) override {
        Check(frame == 0x4000u && name == 0x5001u &&
                  first_flag == 0u && second_flag == 0u,
              "layout arguments");
        calls.emplace_back("load");
        MaybeThrow("load");
    }
    void ConstructMember(std::uint32_t member) override {
        Check(member == 0x4028u, "member offset");
        calls.emplace_back("member");
        MaybeThrow("member");
    }
    void CleanupMember(std::uint32_t member) override {
        Check(member == 0x4028u, "member cleanup offset");
        calls.emplace_back("cleanup-member");
    }
    void SetupInterface(std::uint32_t frame) override {
        Check(frame == 0x4000u, "interface receives frame");
        calls.emplace_back("setup");
        MaybeThrow("setup");
    }
    std::uint32_t DestroyDialog(std::uint32_t frame) override {
        Check(frame == 0x4000u, "base dialog destructor frame");
        calls.emplace_back("destroy-dialog");
        return 0x7777u;
    }
};

void CheckFactory() {
    Runtime runtime;
    runtime.words[pal4::SystemSettingFrame::kNameSource] = 0x5001u;
    Check(pal4::SystemSettingFrame::GetName(runtime) == 0x5001u &&
              runtime.words[pal4::SystemSettingFrame::kNameStorage] ==
                  0x5001u &&
              runtime.calls ==
                  std::vector<std::string>{"read", "write"},
          "name thunk copies SystemSetting pointer into storage");
    runtime.calls.clear();
    Check(pal4::SystemSettingFrame::GetFactory(runtime) == 0x7000u &&
              runtime.event_name == 0x5001u &&
              runtime.event_create ==
                  pal4::SystemSettingFrame::kCreateFunction &&
              runtime.calls == std::vector<std::string>{
                                   "manager", "read", "process"},
          "factory passes stored name and create function");
}

void CheckConstruction() {
    Runtime runtime;
    Check(pal4::SystemSettingFrame::Create(
              0x5001u, runtime) == 0x4000u &&
              runtime.words[0x4028u] ==
                  pal4::SystemSettingFrame::kMemberVtable &&
              runtime.words[0x4000u] ==
                  pal4::SystemSettingFrame::kObjectVtable &&
              runtime.words[0x4040u] == 0x843be8u &&
              runtime.words[0x4060u] == 0x843bd0u &&
              runtime.words[0x404cu] == 0u &&
              runtime.words[0x4050u] == 0u &&
              runtime.words[0x4054u] == 0u &&
              runtime.bytes[0x4048u] == 1u &&
              runtime.calls.front() == "allocate" &&
              runtime.calls.back() == "setup",
          "constructor writes vtables, fields and pointer low byte");
    constexpr std::uint32_t bases[] = {0xf0u, 0x12cu, 0x154u, 0x190u};
    constexpr std::uint32_t counts[] = {3u, 2u, 3u, 2u};
    for (std::size_t group = 0u; group < 4u; ++group) {
        for (std::uint32_t row = 0u; row < counts[group]; ++row) {
            for (std::uint32_t word = 0u; word < 5u; ++word) {
                Check(runtime.words.at(0x4000u + bases[group] +
                                       row * 0x14u + word * 4u) == 0u,
                      "all 50 record words cleared in four groups");
            }
        }
    }

    Runtime empty;
    empty.allocation = 0u;
    Check(pal4::SystemSettingFrame::Create(0x5001u, empty) == 0u &&
              empty.calls == std::vector<std::string>{"allocate"},
          "allocation failure skips constructor");
}

void CheckException(const char* stage,
                    const std::vector<std::string>& expected_tail) {
    Runtime runtime;
    runtime.throw_at = stage;
    bool caught = false;
    try {
        (void)pal4::SystemSettingFrame::Create(0x5001u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.freed.back() == 0x4000u &&
              runtime.calls.size() >= expected_tail.size(),
          "exception frees frame allocation");
    const auto start = runtime.calls.end() -
                       static_cast<std::ptrdiff_t>(expected_tail.size());
    Check(std::vector<std::string>(start, runtime.calls.end()) ==
              expected_tail,
          "construction stage selects correct cleanup sequence");
}

void CheckDestruction() {
    Runtime runtime;
    runtime.words[0x404cu] = 0x9000u;
    runtime.words[0x4030u] = 0xa000u;
    Check(pal4::SystemSettingFrame::Destroy(
              0x4000u, runtime) == 0x7777u &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x9000u, 0xa000u} &&
              runtime.words[0x404cu] == 0u &&
              runtime.words[0x4050u] == 0u &&
              runtime.words[0x4054u] == 0u &&
              runtime.words[0x4030u] == 0u &&
              runtime.words[0x4034u] == 0u &&
              runtime.words[0x4038u] == 0u &&
              runtime.calls.back() == "destroy-dialog",
          "destructor clears both triplets before base destruction");
}

}  // namespace

int main() {
    CheckFactory();
    CheckConstruction();
    CheckException("load", {"load", "free"});
    CheckException("member", {"member", "destroy-dialog", "free"});
    CheckException("setup", {"setup", "read", "free", "write",
                              "write", "write", "cleanup-member",
                              "destroy-dialog", "free"});
    CheckDestruction();
}
