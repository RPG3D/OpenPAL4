#include "pal4/cast_magic_task.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kObject = 0x1000u;

struct Runtime final : pal4::ICastMagicCreationRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = kObject;
    bool fail_base = false;
    bool fail_write = false;
    std::uint32_t vtable = 0u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 76u, "CastMagic factory allocates 76 bytes");
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free receives CastMagic allocation");
        calls.push_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kObject, "base constructed at object start");
        calls.push_back("base");
        if (fail_base) throw std::runtime_error("base failed");
    }
    void DestroyBase(std::uint32_t object) override {
        Check(object == kObject, "base destructor receives object");
        calls.push_back("destroy_base");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == kObject && value == 0x845a64u,
              "CastMagic vtable installed at offset zero");
        calls.push_back("write_vtable");
        if (fail_write) throw std::runtime_error("write failed");
        vtable = value;
    }
};

}  // namespace

int main() {
    try {
        Runtime r;
        Check(pal4::CastMagicTask::Create(r) == kObject &&
                  r.vtable == pal4::CastMagicTask::kVtable &&
                  r.calls == std::vector<std::string>{
                      "allocate", "base", "write_vtable"},
              "factory composes base then derived vtable");

        r = Runtime{};
        r.allocation = 0u;
        Check(pal4::CastMagicTask::Create(r) == 0u &&
                  r.calls == std::vector<std::string>{"allocate"},
              "allocation failure returns before base constructor");

        r = Runtime{};
        r.fail_base = true;
        bool caught = false;
        try { (void)pal4::CastMagicTask::Create(r); }
        catch (const std::runtime_error&) { caught = true; }
        Check(caught && r.calls == std::vector<std::string>{
                  "allocate", "base", "free"},
              "base construction failure frees raw allocation");

        r = Runtime{};
        r.fail_write = true;
        caught = false;
        try { (void)pal4::CastMagicTask::Create(r); }
        catch (const std::runtime_error&) { caught = true; }
        Check(caught && r.calls == std::vector<std::string>{
                  "allocate", "base", "write_vtable",
                  "destroy_base", "free"},
              "derived vtable failure destroys base before free");
        std::cout << "CastMagic factory tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
