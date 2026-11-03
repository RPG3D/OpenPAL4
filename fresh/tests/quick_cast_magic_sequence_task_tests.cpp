#include "pal4/quick_cast_magic_sequence_task.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kChild = 0x2000u;
constexpr std::uint32_t kSource = 0x3000u;
constexpr std::uint32_t kTarget = 0x4000u;
constexpr std::uint32_t kMagic = 0x5000u;

struct Runtime final : pal4::IQuickCastMagicSequenceRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {kTask + 52u, kMagic}, {0x8f9720u, 0x6000u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = kChild;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void NotifySource(std::uint32_t source) override {
        Check(source == kSource, "notified source");
        calls.push_back("notify");
    }
    std::uint32_t CreateChildTask(std::uint32_t factory_vtable) override {
        Check(factory_vtable == pal4::QuickCastMagicChildFactory::kVtable,
              "quick-cast factory vtable");
        calls.push_back("create");
        return allocation;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && child == kChild &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u} &&
                  text == (words[0x8f9720u] == 0u ? 0x841964u : 0x6000u),
              "GOM add arguments");
        calls.push_back("add");
    }
};

}  // namespace

int main() {
    try {
        pal4::QuickCastMagicChildFactorySlot slot{};
        Check(pal4::QuickCastMagicChildFactory::Initialize(&slot) == &slot &&
                  slot.vtable == 0x845a60u,
              "factory slot address and vtable");

        Runtime r;
        Check(pal4::QuickCastMagicSequenceTask::Start(kTask, r) == kMagic &&
                  r.words[kChild + 64u] == kTarget &&
                  r.words[kChild + 60u] == kSource &&
                  r.words[kChild + 68u] == kMagic &&
                  r.calls == std::vector<std::string>{
                      "read:4140", "notify", "read:9410336", "create",
                      "add", "read:4148", "read:4140", "read:4144",
                      "write:8256", "write:8252", "write:8260"},
              "quick-cast order and three child fields");

        r = Runtime{};
        r.words[0x8f9720u] = 0u;
        Check(pal4::QuickCastMagicSequenceTask::Start(kTask, r) == kMagic,
              "empty global text fallback");

        r = Runtime{};
        r.allocation = 0u;
        Check(pal4::QuickCastMagicSequenceTask::Start(kTask, r) == 0u &&
                  r.calls == std::vector<std::string>{
                      "read:4140", "notify", "read:9410336", "create"},
              "null child returns before field writes");
        std::cout << "QuickCastMagic sequence task tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
