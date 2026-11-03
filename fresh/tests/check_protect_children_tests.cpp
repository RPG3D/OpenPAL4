#include "pal4/check_protect_children.h"

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

constexpr std::uint32_t kObject = 0x1000u;

struct Runtime final : pal4::ICheckProtectChildRuntime {
    std::uint32_t allocation = kObject;
    std::uint32_t requested_size = 0u;
    bool throw_on_base = false;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        requested_size = bytes;
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "cleanup frees allocated child");
        calls.push_back("free");
    }
    void ConstructBase(std::uint32_t object) override {
        Check(object == kObject, "base constructor receives child");
        calls.push_back("construct_base");
        if (throw_on_base) throw std::runtime_error("base");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
};

void PositionChild() {
    Runtime runtime;
    Check(pal4::CheckProtectPositionTask::Create(runtime) == kObject &&
              runtime.requested_size == 76u &&
              runtime.words.at(kObject + 72u) == 0u &&
              runtime.words.at(kObject) ==
                  pal4::CheckProtectPositionTask::kVtable &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct_base", "write:4168", "write:4096"},
          "position child sets trailing word before final vtable");
}

void ResponseChild() {
    Runtime runtime;
    Check(pal4::CheckProtectResponseTask::Create(runtime) == kObject &&
              runtime.requested_size == 72u &&
              runtime.words.at(kObject) ==
                  pal4::CheckProtectResponseTask::kVtable &&
              runtime.words.find(kObject + 72u) == runtime.words.end() &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct_base", "write:4096"},
          "response child has only base and derived vtable");
}

void FailureAndUnwind() {
    for (const bool position : {true, false}) {
        Runtime runtime;
        runtime.allocation = 0u;
        const auto create = [&] {
            return position
                ? pal4::CheckProtectPositionTask::Create(runtime)
                : pal4::CheckProtectResponseTask::Create(runtime);
        };
        Check(create() == 0u &&
                  runtime.calls == std::vector<std::string>{"allocate"},
              "allocation failure skips base construction");
        runtime = Runtime{};
        runtime.throw_on_base = true;
        bool caught = false;
        try {
            (void)create();
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.calls == std::vector<std::string>{
                  "allocate", "construct_base", "free"},
              "base construction exception frees child");
    }
}

}  // namespace

int main() {
    PositionChild();
    ResponseChild();
    FailureAndUnwind();
    std::cout << "CheckProtect children tests passed\n";
}
