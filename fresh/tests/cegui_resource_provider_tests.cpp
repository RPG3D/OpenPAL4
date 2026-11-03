#include "pal4/cegui_resource_provider.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiResourceRuntimeStub {
public:
    std::uint32_t allocation_result = 0x4000u;
    std::uint32_t alternate_result = 0x4000u;
    bool throw_in_constructor = false;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocations.push_back(bytes);
        return allocation_result;
    }
    void Free(const std::uint32_t address) override {
        calls.emplace_back("free");
        Check(address == allocation_result,
              "exception cleanup frees allocated stream address");
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        calls.emplace_back("read");
        return pal4::CeguiResourceRuntimeStub::ReadDword(address);
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        calls.emplace_back("write");
        writes.emplace_back(address, value);
        pal4::CeguiResourceRuntimeStub::WriteDword(address, value);
    }
    std::uint32_t DefaultProviderVtable() override {
        calls.emplace_back("default_vtable");
        return 0xaaaau;
    }
    std::uint32_t BaseProviderVtable() override {
        calls.emplace_back("base_vtable");
        return 0xbbbbu;
    }
    void DestroyString(const std::uint32_t address) override {
        calls.emplace_back("destroy_string");
        Check(address == allocation_result + 4u,
              "provider destroys embedded string at +4");
    }
    std::uint32_t ConstructAlternateStream(
        const std::uint32_t address) override {
        calls.emplace_back("alternate_ctor");
        Check(address == allocation_result,
              "alternate constructor receives allocated address");
        if (throw_in_constructor) {
            throw std::runtime_error("alternate construction failed");
        }
        return alternate_result;
    }
    void ConstructDefaultStream(const std::uint32_t address) override {
        calls.emplace_back("default_ctor");
        Check(address == allocation_result,
              "default constructor receives allocated address");
        if (throw_in_constructor) {
            throw std::runtime_error("default construction failed");
        }
    }
};

void ProviderConstructsObservedPrefix() {
    Runtime runtime;
    runtime.WriteDword(0x4000u + 28u, 0xcafebabeu);
    runtime.calls.clear();
    runtime.writes.clear();
    const auto object = pal4::CeguiResourceProvider::Create(runtime);
    Check(object == 0x4000u &&
              runtime.allocations == std::vector<std::uint32_t>{0x9cu},
          "provider allocates exactly 156 bytes");
    Check(runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x4008u, 32u}, {0x400cu, 0u}, {0x4014u, 0u},
                  {0x4010u, 0u}, {0x4004u, 0u}, {0x4018u, 0u},
                  {0x4000u, pal4::CeguiResourceProvider::kVtable}},
          "provider preserves exact observed field-write order");
    Check(runtime.ReadDword(0x401cu) == 0xcafebabeu,
          "provider leaves unobserved object tail untouched");
}

void ProviderAllocationFailureSkipsWrites() {
    Runtime runtime;
    runtime.allocation_result = 0u;
    Check(pal4::CeguiResourceProvider::Create(runtime) == 0u &&
              runtime.writes.empty() &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation returns null without initializing fields");
}

void ProviderDestructorRunsVtablesThenString() {
    Runtime runtime;
    pal4::CeguiResourceProvider::Destroy(0x4000u, runtime);
    Check(runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x4000u, pal4::CeguiResourceProvider::kVtable},
                  {0x4000u, 0xaaaau}, {0x4000u, 0xbbbbu}} &&
              runtime.calls == std::vector<std::string>{
                  "write", "default_vtable", "write",
                  "base_vtable", "write", "destroy_string"},
          "destructor changes vtables in order and destroys string only");
}

void AlternateStreamBranchAndFailure() {
    Runtime runtime;
    runtime.WriteDword(0x5000u + 0x58cu, 1u);
    runtime.calls.clear();
    runtime.writes.clear();
    runtime.alternate_result = 0x6000u;
    Check(pal4::CeguiFileStreamFactory::Create(0x5000u, runtime) ==
              0x6000u &&
              runtime.allocations == std::vector<std::uint32_t>{8u} &&
              runtime.calls == std::vector<std::string>{
                  "read", "allocate", "alternate_ctor"} &&
              runtime.writes.empty(),
          "nonzero game-state flag selects alternate stream constructor");
    runtime.allocation_result = 0u;
    runtime.calls.clear();
    Check(pal4::CeguiFileStreamFactory::Create(0x5000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"read", "allocate"},
          "alternate stream null allocation skips constructor");
}

void DefaultStreamBranchAndFailure() {
    Runtime runtime;
    runtime.WriteDword(0x5000u + 0x58cu, 0u);
    runtime.calls.clear();
    runtime.writes.clear();
    Check(pal4::CeguiFileStreamFactory::Create(0x5000u, runtime) ==
              0x4000u &&
              runtime.calls == std::vector<std::string>{
                  "read", "allocate", "default_ctor", "write"} &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x4000u, pal4::CeguiFileStreamFactory::kDefaultVtable}},
          "zero flag constructs default stream and replaces vtable");
    runtime.allocation_result = 0u;
    runtime.calls.clear();
    Check(pal4::CeguiFileStreamFactory::Create(0x5000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"read", "allocate"},
          "default stream null allocation skips constructor");
}

void BothConstructorFailuresFreeAllocatedMemory() {
    for (const auto alternate : {false, true}) {
        Runtime runtime;
        runtime.WriteDword(0x5000u + 0x58cu, alternate ? 1u : 0u);
        runtime.calls.clear();
        runtime.throw_in_constructor = true;
        bool caught = false;
        try {
            static_cast<void>(pal4::CeguiFileStreamFactory::Create(
                0x5000u, runtime));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.calls.back() == "free" &&
                  runtime.calls.size() == 4u,
              "either constructor exception releases allocated stream");
    }
}

}  // namespace

int main() {
    ProviderConstructsObservedPrefix();
    ProviderAllocationFailureSkipsWrites();
    ProviderDestructorRunsVtablesThenString();
    AlternateStreamBranchAndFailure();
    DefaultStreamBranchAndFailure();
    BothConstructorFailuresFreeAllocatedMemory();
    std::cout << "6 CEGUI resource provider test cases passed\n";
}
