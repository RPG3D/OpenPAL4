#include "pal4/save_window_callback_object.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::SaveWindowCallbackObjectRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation{0x7000u};

    std::uint32_t Allocate(std::size_t bytes) override {
        Check(bytes == 0x18u, "clone allocates 0x18 bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    void DestroyBase(std::uint32_t object) override {
        Check(object == 0x7000u, "base callback destructor target");
        calls.emplace_back("destroy-base");
    }
    void Free(std::uint32_t object) override {
        Check(object == 0x7000u, "deleting callback storage");
        calls.emplace_back("free");
    }
};

}  // namespace

int main() {
    Runtime runtime;
    runtime.words[0x5008u] = 0x1234u;
    runtime.words[0x500cu] = 0x5678u;
    runtime.words[0x5010u] = 0x9abcu;
    Check(pal4::SaveWindowCallbackObject::Clone(0x5000u, runtime) ==
              0x7000u && runtime.words.at(0x7000u) == 0x8437b8u &&
              runtime.words.at(0x7008u) == 0x1234u &&
              runtime.words.at(0x700cu) == 0x5678u &&
              runtime.words.at(0x7010u) == 0x9abcu &&
              runtime.calls.size() == 9u &&
              runtime.calls[1] == "write:28672:8664528" &&
              runtime.calls.back() == "write:28672:8665016",
          "clone writes base vtable, copies three words, then final vtable");
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::SaveWindowCallbackObject::Clone(0x5000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation does not read source");
    runtime = Runtime{};
    Check(pal4::SaveWindowCallbackObject::Destroy(
              0x7000u, 0u, runtime) == 0x7000u &&
              runtime.calls == std::vector<std::string>{"destroy-base"},
          "non-deleting destructor retains storage");
    runtime.calls.clear();
    Check(pal4::SaveWindowCallbackObject::Destroy(
              0x7000u, 3u, runtime) == 0x7000u &&
              runtime.calls == std::vector<std::string>{
                  "destroy-base", "free"},
          "deleting flag bit zero frees storage after base destructor");
    std::cout << "Save window callback object tests passed\n";
}
