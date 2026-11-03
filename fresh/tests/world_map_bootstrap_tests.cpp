#include "pal4/world_map_bootstrap.h"
#include "pal4/world_map_frame.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IWorldMapBootstrapRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{0x8b089cu, 0x7000u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000u;
    bool fail_construct = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 0x274u, "world-map allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ConstructMap(const std::uint32_t storage,
                               const std::uint32_t name) override {
        Check(storage == allocation && name == 0x7000u,
              "map constructor arguments");
        calls.emplace_back("construct");
        if (fail_construct) throw std::runtime_error("construct");
        return 0x5004u;
    }
    void Free(const std::uint32_t storage) override {
        Check(storage == allocation, "failed map allocation");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    std::uint32_t ProcessFrameEvent(const std::uint32_t manager,
                                    const std::uint32_t name,
                                    const std::uint32_t factory) override {
        Check(manager == 0x6000u && name == 0x7000u &&
                  factory == 0x483b70u,
              "map factory arguments");
        calls.emplace_back("process");
        return 0x8000u;
    }
};

struct FrameRuntime final : pal4::IWorldMapFrameRuntime {
    std::vector<std::string> calls;
    bool fail_events = false;
    void LoadLayout(std::uint32_t frame, std::uint32_t name) override {
        Check(frame == 0x5000u && name == 0x7000u,
              "composed world-map layout arguments");
        calls.emplace_back("layout");
    }
    void DestroyBase(std::uint32_t) override {
        calls.emplace_back("destroy base");
    }
    void ConstructMouse(std::uint32_t) override {
        calls.emplace_back("mouse");
    }
    void DestroyMouse(std::uint32_t) override {
        calls.emplace_back("destroy mouse");
    }
    void ConstructNames(std::uint32_t, std::uint32_t) override {
        calls.emplace_back("names");
    }
    void DestroyNames(std::uint32_t, std::uint32_t) override {
        calls.emplace_back("destroy names");
    }
    std::uint8_t ReadByte(std::uint32_t) override { return 0x57u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void ClearNameFlag(std::uint32_t) override {}
    void AssignText(std::uint32_t, std::string_view) override {}
    void DestroyText(std::uint32_t) override {
        calls.emplace_back("destroy fly");
    }
    void InitializeEventHandlers(std::uint32_t) override {
        calls.emplace_back("events");
        if (fail_events) throw std::runtime_error("events");
    }
    bool AllocateText(std::uint32_t, std::uint32_t) override {
        return true;
    }
    void CopyText(std::uint32_t, std::string_view) override {}
    void SetTextSize(std::uint32_t, std::uint32_t) override {}
};

void TestNameAndFactory() {
    Runtime runtime;
    Check(pal4::WorldMapBootstrap::GetFrameName(runtime) == 0x7000u &&
              runtime.words.at(0x8e7990u) == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                  "read:9111708", "write:9337232"},
          "name thunk copies world-map name pointer");
    runtime.calls.clear();
    Check(pal4::WorldMapBootstrap::InitializeFrameName(runtime) ==
              0x7000u && runtime.calls.size() == 2u,
          "name initializer is directly callable");
    runtime.calls.clear();
    Check(pal4::WorldMapBootstrap::GetFactory(runtime) == 0x8000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read:9337232", "process"},
          "factory thunk calls manager with saved name and callback");
    runtime.calls.clear();
    Check(pal4::WorldMapBootstrap::SetupFrame(runtime) == 0x8000u &&
              runtime.calls.size() == 3u,
          "factory setup is directly callable");
}

void TestCreate() {
    Runtime runtime;
    Check(pal4::WorldMapBootstrap::CreateMap(0x7000u, runtime) ==
              0x5004u && runtime.calls == std::vector<std::string>{
                  "allocate", "construct"},
          "factory returns map constructor result");
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::WorldMapBootstrap::CreateMap(0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation skips constructor");
    runtime = Runtime{};
    runtime.fail_construct = true;
    bool caught = false;
    try {
        (void)pal4::WorldMapBootstrap::CreateMap(0x7000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "construct", "free"},
          "constructor exception frees map allocation");
}

void TestComposedCreate() {
    Runtime runtime;
    FrameRuntime frame;
    Check(pal4::WorldMapBootstrap::CreateMap(0x7000u, runtime, frame) ==
              0x5000u &&
              runtime.calls == std::vector<std::string>{"allocate"} &&
              frame.calls == std::vector<std::string>{
                  "layout", "mouse", "names", "events"},
          "factory composes reconstructed world-map constructor");

    runtime = Runtime{};
    frame = FrameRuntime{};
    runtime.allocation = 0u;
    Check(pal4::WorldMapBootstrap::CreateMap(0x7000u, runtime, frame) ==
              0u && frame.calls.empty(),
          "null allocation skips composed world-map constructor");

    runtime = Runtime{};
    frame = FrameRuntime{};
    frame.fail_events = true;
    bool caught = false;
    try {
        (void)pal4::WorldMapBootstrap::CreateMap(0x7000u,
                                                  runtime, frame);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate", "free"} &&
              frame.calls == std::vector<std::string>{
                  "layout", "mouse", "names", "events", "destroy fly",
                  "destroy names", "destroy mouse", "destroy base"},
          "composed constructor unwinds and factory frees storage");
}

}  // namespace

int main() {
    TestNameAndFactory();
    TestCreate();
    TestComposedCreate();
    return EXIT_SUCCESS;
}
