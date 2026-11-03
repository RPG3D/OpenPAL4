#include "pal4/world_map_registration.h"

#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::IWorldMapRegistrationRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000u;
    bool fail_insert = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x8b089cu, "world-map name source");
        calls.emplace_back("name");
        return 0x7000u;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 8u, "world-map handler allocation");
        calls.emplace_back("allocate");
        return allocation;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check((address == 0x5004u && value == 0x7000u) ||
              (address == 0x5000u && value == 0x843494u),
              "handler name and vtable");
        calls.emplace_back(address == 0x5004u ? "write-name" :
                                               "write-vtable");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    void InsertOrReplace(const std::uint32_t tree,
                         const std::uint32_t key,
                         const std::uint32_t handler) override {
        Check(tree == 0x6104u && key == 0x8a80c0u &&
                  handler == allocation,
              "show_world_map tree insertion");
        calls.emplace_back("insert");
        if (fail_insert) throw std::runtime_error("insert");
    }
    void ReleaseHandler(const std::uint32_t handler,
                        const std::uint32_t flags) override {
        Check(handler == allocation && flags == 1u,
              "temporary world-map handler release");
        calls.emplace_back("release");
    }
};

void TestNormal() {
    Runtime runtime;
    Check(pal4::WorldMapRegistration::GetHandler(runtime) ==
              0x8e798cu &&
              runtime.calls == std::vector<std::string>{
                  "name", "allocate", "write-name", "write-vtable",
                  "manager", "insert", "release"},
          "thunk registers world-map event and releases temporary handler");
    runtime.calls.clear();
    Check(pal4::WorldMapRegistration::RegisterHandler(runtime) ==
              0x8e798cu && runtime.calls.size() == 7u,
          "registration entry is directly callable");
}

void TestNullAndException() {
    Runtime empty;
    empty.allocation = 0u;
    Check(pal4::WorldMapRegistration::RegisterHandler(empty) ==
              0x8e798cu &&
              empty.calls == std::vector<std::string>{
                  "name", "allocate", "manager", "insert"},
          "null allocation inserts null handler without release");
    Runtime failed;
    failed.fail_insert = true;
    bool caught = false;
    try {
        (void)pal4::WorldMapRegistration::RegisterHandler(failed);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed.calls.back() == "release",
          "insertion failure releases temporary handler");
}

}  // namespace

int main() {
    TestNormal();
    TestNullAndException();
    return EXIT_SUCCESS;
}
