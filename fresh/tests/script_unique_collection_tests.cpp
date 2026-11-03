#include "pal4/script_unique_collection.h"

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

class Runtime final : public pal4::IScriptUniqueCollectionRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::uint32_t next_address = 0x1000u;
    bool fail_allocation = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto item = memory.find(address);
        Check(item != memory.end(), "collection reads initialized element");
        return item->second;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        memory[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate:" + std::to_string(bytes));
        if (fail_allocation) return 0u;
        const auto address = next_address;
        next_address += bytes + 0x100u;
        return address;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
};

void Cases() {
    Runtime runtime;
    pal4::ScriptUniqueCollection collection(runtime);
    pal4::ScriptUniqueCollectionState state;
    state.At(0u) = 0xdeadbeefu;

    Check(collection.AddUnique(state, 0x11u) == 0u &&
              state.At(4u) == 0x1000u && state.At(8u) == 0x1004u &&
              state.At(12u) == 0x1004u &&
              runtime.ReadWord(0x1000u) == 0x11u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:4", "write:4096", "free:0"},
          "empty collection allocates one element and returns old size");

    runtime.calls.clear();
    Check(collection.AddUnique(state, 0x11u) == 0x1004u &&
              state.At(8u) == 0x1004u && runtime.calls.empty(),
          "duplicate returns old end without modifying collection");

    Check(collection.AddUnique(state, 0x22u) == 1u &&
              state.At(4u) == 0x1104u && state.At(8u) == 0x110cu &&
              state.At(12u) == 0x110cu &&
              runtime.ReadWord(0x1104u) == 0x11u &&
              runtime.ReadWord(0x1108u) == 0x22u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:8", "write:4356", "write:4360", "free:4096"},
          "full one-element buffer doubles and preserves insertion order");

    runtime.calls.clear();
    Check(collection.AddUnique(state, 0x33u) == 2u &&
              state.At(4u) == 0x120cu && state.At(8u) == 0x1218u &&
              state.At(12u) == 0x121cu &&
              runtime.ReadWord(0x120cu) == 0x11u &&
              runtime.ReadWord(0x1210u) == 0x22u &&
              runtime.ReadWord(0x1214u) == 0x33u,
          "full two-element buffer grows to four slots");

    runtime.calls.clear();
    const auto old_end = state.At(8u);
    Check(collection.AddUnique(state, 0x44u) == old_end &&
              state.At(8u) == old_end + 4u &&
              state.At(12u) == 0x121cu &&
              runtime.ReadWord(old_end) == 0x44u &&
              runtime.calls == std::vector<std::string>{"write:4632"},
          "spare capacity appends in place and returns old end pointer");

    runtime.fail_allocation = true;
    runtime.calls.clear();
    const auto old_state = state.words;
    bool failed = false;
    try {
        static_cast<void>(collection.AddUnique(state, 0x55u));
    } catch (const std::bad_alloc&) {
        failed = true;
    }
    Check(failed && state.words == old_state &&
              runtime.calls == std::vector<std::string>{"allocate:32"} &&
              state.At(0u) == 0xdeadbeefu,
          "failed growth preserves buffer pointers and unrelated header");

    runtime.calls.clear();
    collection.Remove(state, 0x11u);
    Check(state.At(8u) == 0x1218u &&
              state.At(12u) == 0x121cu &&
              runtime.ReadWord(0x120cu) == 0x22u &&
              runtime.ReadWord(0x1210u) == 0x33u &&
              runtime.ReadWord(0x1214u) == 0x44u &&
              runtime.calls == std::vector<std::string>{
                  "write:4620", "write:4624", "write:4628"},
          "remove first value shifts following elements and keeps capacity");

    runtime.calls.clear();
    collection.Remove(state, 0x33u);
    Check(state.At(8u) == 0x1214u &&
              runtime.ReadWord(0x1210u) == 0x44u &&
              runtime.calls == std::vector<std::string>{"write:4624"},
          "remove middle value shifts only its suffix");

    runtime.calls.clear();
    collection.Remove(state, 0x44u);
    Check(state.At(8u) == 0x1210u && runtime.calls.empty(),
          "remove last value only decreases end pointer");
    collection.Remove(state, 0x99u);
    Check(state.At(8u) == 0x1210u && runtime.calls.empty(),
          "missing value leaves collection untouched");
    collection.Remove(state, 0x22u);
    Check(state.At(8u) == state.At(4u),
          "remove sole element produces empty collection");
    collection.Remove(state, 0x22u);
    Check(state.At(8u) == state.At(4u) &&
              state.At(12u) == 0x121cu,
          "remove from empty collection preserves buffer and capacity");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
