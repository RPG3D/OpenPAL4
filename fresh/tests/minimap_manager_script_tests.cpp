#include "pal4/minimap_manager.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapManagerScriptRuntime {
public:
    pal4::MinimapManagerObjectState state;
    std::vector<std::uint32_t> layers;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocation = 0x6000u;

    pal4::MinimapManagerObjectState& Access(std::uint32_t address) override {
        Check(address == 0x1000u, "manager state address");
        calls.emplace_back("access");
        return state;
    }
    void Reset(std::uint32_t manager) override {
        Check(manager == 0x1000u, "reset manager address");
        calls.emplace_back("reset");
        state.WriteDword(0x1cu, state.ReadDword(0x18u));
    }
    std::vector<std::uint32_t> LoadLayerRecords(
        const std::string& folder, const std::string& scene,
        std::uint8_t incoming_byte) override {
        Check(folder == "Maps" && scene == "Scene01" &&
                  incoming_byte == 0x5au,
              "layer loader arguments");
        calls.emplace_back("load");
        return layers;
    }
    void WriteListWord(std::uint32_t address,
                       std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t AllocateList(std::uint32_t bytes) override {
        Check(bytes == layers.size() * 4u, "allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void FreeList(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    void SelectTexture(std::uint32_t manager,
                       std::uint32_t index) override {
        Check(manager == 0x1000u && index == 0u,
              "first minimap layer selection");
        calls.emplace_back("select");
    }
};

bool Load(Runtime& runtime) {
    return pal4::MinimapManagerObject::LoadScriptData(
        0x1000u, "Maps", "Scene01", 0x5au, runtime);
}

void TestNewList() {
    Runtime runtime;
    runtime.layers = {0xaaaau, 0xbbbbu};
    Check(Load(runtime), "loaded layers return success");
    Check(runtime.calls == std::vector<std::string>{
              "reset", "load", "access", "free:0", "allocate",
              "write:24576", "write:24580", "select"},
          "new list allocation, copy and selection order");
    Check(runtime.state.ReadDword(0x18u) == 0x6000u &&
              runtime.state.ReadDword(0x1cu) == 0x6008u &&
              runtime.state.ReadDword(0x20u) == 0x6008u &&
              runtime.words[0x6000u] == 0xaaaau &&
              runtime.words[0x6004u] == 0xbbbbu,
          "new list begin, end, capacity and contents");
}

void TestReuseCapacity() {
    Runtime runtime;
    runtime.layers = {10u, 20u, 30u};
    runtime.state.WriteDword(0x18u, 0x5000u);
    runtime.state.WriteDword(0x1cu, 0x5008u);
    runtime.state.WriteDword(0x20u, 0x5010u);
    runtime.words[0x500cu] = 0xddddu;
    Check(Load(runtime), "reused list returns success");
    Check(runtime.state.ReadDword(0x18u) == 0x5000u &&
              runtime.state.ReadDword(0x1cu) == 0x500cu &&
              runtime.state.ReadDword(0x20u) == 0x5010u &&
              runtime.words[0x5000u] == 10u &&
              runtime.words[0x5004u] == 20u &&
              runtime.words[0x5008u] == 30u &&
              runtime.words[0x500cu] == 0xddddu,
          "capacity reuse preserves allocation and trailing slot");
    Check(runtime.calls == std::vector<std::string>{
              "reset", "load", "access", "write:20480",
              "write:20484", "write:20488", "select"},
          "reuse does not allocate or free");
}

void TestReplaceInsufficientCapacity() {
    Runtime runtime;
    runtime.layers = {1u, 2u};
    runtime.state.WriteDword(0x18u, 0x5000u);
    runtime.state.WriteDword(0x1cu, 0x5004u);
    runtime.state.WriteDword(0x20u, 0x5004u);
    Check(Load(runtime) &&
              runtime.state.ReadDword(0x18u) == 0x6000u &&
              runtime.state.ReadDword(0x1cu) == 0x6008u &&
              runtime.state.ReadDword(0x20u) == 0x6008u,
          "insufficient capacity replaces old allocation");
    Check(runtime.calls == std::vector<std::string>{
              "reset", "load", "access", "free:20480", "allocate",
              "write:24576", "write:24580", "select"},
          "old allocation freed before replacement");
}

void TestEmpty() {
    Runtime runtime;
    Check(!Load(runtime) &&
              runtime.calls ==
                  std::vector<std::string>{"reset", "load", "access"},
          "empty loader result leaves null list and returns false");
    runtime.calls.clear();
    runtime.state.WriteDword(0x18u, 0x5000u);
    runtime.state.WriteDword(0x1cu, 0x5008u);
    runtime.state.WriteDword(0x20u, 0x5010u);
    Check(!Load(runtime) &&
              runtime.state.ReadDword(0x18u) == 0x5000u &&
              runtime.state.ReadDword(0x1cu) == 0x5000u &&
              runtime.state.ReadDword(0x20u) == 0x5010u,
          "empty result retains reusable storage but has zero size");
    Check(runtime.calls ==
              std::vector<std::string>{"reset", "load", "access"},
          "empty result does not select texture");
}

}  // namespace

int main() {
    TestNewList();
    TestReuseCapacity();
    TestReplaceInsufficientCapacity();
    TestEmpty();
    return EXIT_SUCCESS;
}
