#include "pal4/magic_window_event_callback.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowEventCallbackRuntimeStub {
    std::uint32_t allocation{0x7000u};
    std::map<std::uint32_t, std::uint32_t> source{
        {0x6008u, 0x11u}, {0x600cu, 0x22u}, {0x6010u, 0x33u}};
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "callback clone allocation size");
        calls.push_back("allocate");
        return allocation;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return source.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        writes.emplace_back(address, value);
    }
};

void TestClone() {
    Runtime runtime;
    Check(pal4::MagicWindowEventCallback::Clone(0x6000u, runtime) == 0x7000u,
          "clone returns allocated address");
    Check(runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x7000u, pal4::MagicWindowEventCallback::kProvisionalVtable},
                  {0x7008u, 0x11u}, {0x700cu, 0x22u}, {0x7010u, 0x33u},
                  {0x7000u, pal4::MagicWindowEventCallback::kCloneVtable}},
          "clone copies three fields and finalizes vtable");
    Check(runtime.calls == std::vector<std::string>{
                               "allocate", "write:28672", "read:24584",
                               "write:28680", "read:24588", "write:28684",
                               "read:24592", "write:28688", "write:28672"},
          "clone preserves IDA read/write order");

    Runtime failed;
    failed.allocation = 0;
    Check(pal4::MagicWindowEventCallback::Clone(0x6000u, failed) == 0u &&
              failed.calls == std::vector<std::string>{"allocate"},
          "allocation failure avoids source reads and writes");
}

void TestPanelConstructor() {
    Runtime runtime;
    Check(pal4::MagicWindowEventCallback::ConstructPanel(
              0x8000u, 0x45a670u, 3u, 0x4000u, runtime) == 0x8000u,
          "constructor returns storage address");
    Check(runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x8008u, 0x45a670u}, {0x800cu, 3u},
                  {0x8010u, 0x4000u},
                  {0x8000u, pal4::MagicWindowEventCallback::kPanelVtable}},
          "constructor writes three arguments before panel vtable");
    Check(runtime.calls.size() == 4u, "constructor does not allocate or read");
}

}  // namespace

int main() {
    TestClone();
    TestPanelConstructor();
}
