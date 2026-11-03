#include "pal4/picture_view_window_lifecycle.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PictureViewWindowLifecycleRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> read_records;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    pal4::PictureRecordRange range{0x9000u, 0x9000u + 6u * 0x34u};
    std::uint32_t next_allocation = 0xa000u;
    std::uint32_t fail_allocation_index = 0xffffffffu;
    bool fail_elements{};

    Runtime() {
        constexpr std::array<std::uint32_t, 6> ids{{
            99u, 100u, 101u, 0xffffffffu, 150u, 102u}};
        for (std::uint32_t i = 0u; i < ids.size(); ++i)
            words[0x9000u + i * 0x34u] = ids[i];
    }
    void LoadLayout(pal4::PictureViewWindowState&,
                    std::uint32_t name, std::uint32_t first,
                    std::uint32_t second) override {
        Check(name == 0x2042u && first == 0u && second == 0u,
              "picture dialog layout receives name and two zero flags");
        calls.emplace_back("layout");
    }
    void InitializeElements(pal4::PictureViewWindowState& state) override {
        Check(state.Read(0u) ==
                  pal4::PictureViewWindowLifecycle::kObjectVtable &&
                  state.Read(0x28u) ==
                      pal4::PictureViewWindowLifecycle::kMemberVtable &&
                  state.bytes[0x2cu] == 0x10u &&
                  state.bytes[0x50u] == 0x42u &&
                  state.Read(0x30u) == 0u &&
                  state.Read(0x54u) == 0u &&
                  state.Read(0x58u) == 0u &&
                  state.Read(0x5cu) == 0u,
              "elements see native member/vector initialization and name low byte");
        calls.emplace_back("elements");
        if (fail_elements) throw std::runtime_error("elements");
    }
    pal4::PictureRecordRange PictureRecords() override {
        calls.emplace_back("picture-range");
        return range;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address >= 0x9000u && address < range.end)
            read_records.push_back(address);
        const auto it = words.find(address);
        Check(it != words.end(), "read targets record ID or populated vector slot");
        return it->second;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        if (allocations.size() == fail_allocation_index) return 0u;
        allocations.push_back(bytes);
        const auto address = next_allocation;
        next_allocation += 0x1000u;
        return address;
    }
    void FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
    }
    std::uint32_t DestroyDialog(
        pal4::PictureViewWindowState&) override {
        calls.emplace_back("destroy-dialog");
        return 0xcafeu;
    }
};

void CheckNormal() {
    using Lifecycle = pal4::PictureViewWindowLifecycle;
    pal4::PictureViewWindowState state{};
    Runtime runtime;
    Check(&Lifecycle::Construct(state, 0x1000u, 0x2042u, runtime) ==
                  &state &&
              runtime.calls == std::vector<std::string>{
                  "layout", "elements", "picture-range"} &&
              runtime.read_records == std::vector<std::uint32_t>{
                  0x9000u, 0x9034u, 0x9068u, 0x909cu,
                  0x90d0u, 0x9104u} &&
              runtime.allocations == std::vector<std::uint32_t>{
                  4u, 8u, 16u} &&
              state.Read(0x54u) == 0xc000u &&
              state.Read(0x58u) == 0xc010u &&
              state.Read(0x5cu) == 0xc010u &&
              state.Read(0x60u) == 0u,
          "constructor filters signed IDs and grows record vector 1/2/4");
    Check(runtime.words.at(0xc000u) == 0x9034u &&
              runtime.words.at(0xc004u) == 0x9068u &&
              runtime.words.at(0xc008u) == 0x90d0u &&
              runtime.words.at(0xc00cu) == 0x9104u &&
              runtime.freed == std::vector<std::uint32_t>{
                  0u, 0xa000u, 0xb000u},
          "vector keeps eligible record addresses and frees prior buffers");
    state.Write(0x30u, 0xd000u);
    Check(Lifecycle::Cleanup(state, runtime) == 0xcafeu &&
              runtime.freed == std::vector<std::uint32_t>{
                  0u, 0xa000u, 0xb000u, 0xc000u, 0xd000u} &&
              state.Read(0x30u) == 0u &&
              state.Read(0x34u) == 0u &&
              state.Read(0x38u) == 0u &&
              state.Read(0x54u) == 0u &&
              state.Read(0x58u) == 0u &&
              state.Read(0x5cu) == 0u &&
              runtime.calls.back() == "destroy-dialog",
          "cleanup frees vector then member and destroys dialog base");
    Check(!pal4::PictureViewWindowRecords::Eligible(99u) &&
              pal4::PictureViewWindowRecords::Eligible(100u) &&
              !pal4::PictureViewWindowRecords::Eligible(0xffffffffu),
          "record eligibility uses signed compare against 100");
}

void CheckFailures() {
    pal4::PictureViewWindowState state{};
    Runtime runtime;
    runtime.range = {};
    Check(&pal4::PictureViewWindowLifecycle::Construct(
              state, 0x1000u, 0x2042u, runtime) == &state &&
              runtime.allocations.empty() &&
              runtime.read_records.empty() &&
              state.Read(0x60u) == 0u,
          "null database begin skips record traversal");

    state = {};
    runtime = Runtime{};
    runtime.fail_elements = true;
    try {
        (void)pal4::PictureViewWindowLifecycle::Construct(
            state, 0x1000u, 0x2042u, runtime);
        Check(false, "element initialization failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "elements", "destroy-dialog"} &&
                  runtime.freed == std::vector<std::uint32_t>{0u, 0u},
              "element failure unwinds empty vector, member and dialog");
    }

    state = {};
    runtime = Runtime{};
    runtime.fail_allocation_index = 1u;
    try {
        (void)pal4::PictureViewWindowLifecycle::Construct(
            state, 0x1000u, 0x2042u, runtime);
        Check(false, "vector allocation failure must propagate");
    } catch (const std::bad_alloc&) {
        Check(runtime.allocations == std::vector<std::uint32_t>{4u} &&
                  runtime.freed == std::vector<std::uint32_t>{
                      0u, 0xa000u, 0u} &&
                  state.Read(0x54u) == 0u &&
                  runtime.calls.back() == "destroy-dialog",
              "failed growth frees completed vector and member");
    }
}

void CheckCopyWords() {
    Runtime runtime;
    runtime.words[0x2000u] = 11u;
    runtime.words[0x2004u] = 22u;
    Check(pal4::PictureViewWindowRecords::CopyWords(
              0x2000u, 0x2008u, 0x3000u, runtime) == 0x3008u &&
              runtime.words.at(0x3000u) == 11u &&
              runtime.words.at(0x3004u) == 22u,
          "DWORD copy returns destination end and preserves values");
    Check(pal4::PictureViewWindowRecords::CopyWords(
              0x2000u, 0x2008u, 0u, runtime) == 8u &&
              runtime.words.find(0u) == runtime.words.end() &&
              runtime.words.at(4u) == 22u,
          "zero destination skips first write but still advances");
    Check(pal4::PictureViewWindowRecords::CopyWords(
              0x2000u, 0x2000u, 0x3000u, runtime) == 0x3000u,
          "empty DWORD range returns unchanged destination");
}

}  // namespace

int main() {
    CheckNormal();
    CheckFailures();
    CheckCopyWords();
    std::cout << "Picture view window lifecycle tests passed\n";
}
