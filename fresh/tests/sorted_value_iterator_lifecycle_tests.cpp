#include "pal4/sorted_value_iterator_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISortedValueIteratorLifecycleRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> frees;
    std::uint32_t allocation{0x8000u};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        frees.push_back(address);
    }
};

using Lifecycle = pal4::SortedValueIteratorLifecycle;

void TestValidClone() {
    Runtime runtime;
    runtime.words[0x5004u] = 0x1234u;
    runtime.words[0x5008u] = 0x5678u;
    Check(Lifecycle::CloneValid(0x5000u, runtime) == 0x8000u &&
              runtime.words[0x8000u] == Lifecycle::kValidVtable &&
              runtime.words[0x8004u] == 0x1234u &&
              runtime.words[0x8008u] == 0x5678u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:12", "write:32768:8662100", "read:20484",
                  "write:32772:4660", "read:20488", "write:32776:22136",
                  "write:32768:8662012"},
          "valid iterator clone writes base vtable then fields then valid vtable");

    runtime = {};
    runtime.allocation = 0u;
    Check(Lifecycle::CloneValid(0x5000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:12"},
          "valid clone allocation failure skips source reads");
}

void TestInvalidClone() {
    Runtime runtime;
    runtime.words[0x5004u] = 0xabcdefu;
    Check(Lifecycle::CloneInvalid(0x5000u, runtime) == 0x8000u &&
              runtime.words[0x8000u] == Lifecycle::kInvalidVtable &&
              runtime.words[0x8004u] == 0xabcdefu &&
              runtime.calls.size() == 5u &&
              runtime.calls.front() == "allocate:8" &&
              runtime.calls.back() == "write:32768:8662056",
          "invalid iterator clone copies one field and finalizes its vtable");

    runtime = {};
    runtime.allocation = 0u;
    Check(Lifecycle::CloneInvalid(0x5000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:8"},
          "invalid clone allocation failure skips source reads");
}

void TestAccessAndDeletion() {
    Runtime runtime;
    Check(Lifecycle::FieldAddress(0x5000u) == 0x5004u &&
              Lifecycle::FieldAddress(0xffffffffu) == 3u,
          "field accessor returns this +4 with 32-bit wraparound");
    Lifecycle::SetBaseVtable(0x5000u, runtime);
    Check(runtime.words[0x5000u] == Lifecycle::kBaseVtable &&
              runtime.calls.size() == 1u,
          "base-vtable setter writes only the first DWORD");

    runtime.calls.clear();
    Check(Lifecycle::DeleteBase(0x5000u, 2u, runtime) == 0x5000u &&
              runtime.words[0x5000u] == Lifecycle::kBaseVtable &&
              runtime.frees.empty() && runtime.calls.size() == 1u,
          "base deleting destructor retains storage when bit zero is clear");
    runtime.calls.clear();
    Check(Lifecycle::DeleteBase(0x5000u, 1u, runtime) == 0x5000u &&
              runtime.frees == std::vector<std::uint32_t>{0x5000u} &&
              runtime.calls.back() == "free:20480",
          "base deleting destructor frees when bit zero is set");

    runtime.calls.clear();
    Check(Lifecycle::DeleteDerived(0x6000u, 0u, runtime) == 0x6000u &&
              runtime.words[0x6000u] == Lifecycle::kBaseVtable &&
              runtime.calls.size() == 1u,
          "derived deleting destructor resets base vtable without freeing");
    Check(Lifecycle::DeleteDerived(0x6000u, 3u, runtime) == 0x6000u &&
              runtime.frees.back() == 0x6000u,
          "derived deleting destructor frees when bit zero is set");
}

}  // namespace

int main() {
    TestValidClone();
    TestInvalidClone();
    TestAccessAndDeletion();
    pal4::SortedValueIteratorLifecycleRuntimeStub stub;
    Check(Lifecycle::CloneValid(0u, stub) == 0u &&
              Lifecycle::CloneInvalid(0u, stub) == 0u,
          "default iterator lifecycle stub links");
    std::cout << "4 iterator lifecycle cases passed\n";
}
