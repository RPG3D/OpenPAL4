#include "pal4/scene_evf_record.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISceneEvfListRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> freed;
    std::uint32_t ReadListWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteListWord(std::uint32_t address,
                       std::uint32_t value) override {
        words[address] = value;
    }
    void FreeListNode(std::uint32_t address) override {
        freed.push_back(address);
    }
};

}  // namespace

int main() {
    pal4::SceneEvfRecordState state;
    state.words[39u] = 0x1000u;
    Runtime runtime;
    runtime.words[0x1000u] = 0x1000u;
    runtime.words[0x1004u] = 0x1000u;
    pal4::SceneEvfRecord::ClearLinkedList(state, runtime);
    Check(runtime.freed.empty() && state.words[40u] == 0u,
          "empty sentinel remains linked");

    state.words[40u] = 2u;
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x1004u] = 0x3000u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x1000u;
    runtime.words[0x3000u] = 0x1000u;
    runtime.words[0x3004u] = 0x2000u;
    pal4::SceneEvfRecord::ClearLinkedList(state, runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x2000u, 0x3000u} &&
              runtime.words[0x1000u] == 0x1000u &&
              runtime.words[0x1004u] == 0x1000u &&
              state.words[40u] == 0u,
          "two nodes unlink in forward order and sentinel remains");
}
