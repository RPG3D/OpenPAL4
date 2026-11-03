#include "pal4/role_state_receiver_names.h"

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

struct Runtime final : pal4::IPreviewTitleStringRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> heap;
    bool use_heap = false;

    void Grow(pal4::PreviewTitleStringState& state,
              std::uint32_t length) override {
        calls.push_back("grow:" + std::to_string(length));
        if (use_heap) {
            state.Write(4u, 33u);
            state.Write(0x94u, 0x9000u);
        }
    }
    void WriteCodepoint(std::uint32_t address,
                        std::uint32_t codepoint) override {
        heap[address] = codepoint;
        calls.push_back("codepoint:" + std::to_string(address));
    }
    void Destroy(pal4::PreviewTitleStringState&) override {
        calls.emplace_back("destroy");
    }
    int RegisterAtExit(std::uint32_t callback) override {
        calls.push_back("atexit:" + std::to_string(callback));
        return 23;
    }
};

std::string InlineText(const pal4::PreviewTitleStringState& state) {
    std::string result;
    for (std::uint32_t offset = 0x14u; offset < 0x94u; offset += 4u) {
        const auto codepoint = state.Read(offset);
        if (codepoint == 0u) break;
        result.push_back(static_cast<char>(codepoint));
    }
    return result;
}

void TestDragReceiver() {
    Runtime runtime;
    pal4::PreviewTitleStringState state;
    Check(pal4::RoleStateReceiverNames::InitializeDrag(state, runtime) ==
              23 && state.Read(0u) == 12u &&
              state.Read(4u) == 32u &&
              InlineText(state) == "DragReceiver" &&
              runtime.calls == std::vector<std::string>{
                  "grow:12", "atexit:4669776"},
          "drag name initialization and cleanup registration");
    pal4::RoleStateReceiverNames::DestroyDrag(state, runtime);
    Check(runtime.calls.back() == "destroy", "drag name destructor");

    pal4::PreviewTitleStringState direct;
    runtime.calls.clear();
    Check(pal4::RoleStateReceiverNames::CreateDrag(direct, runtime) ==
              pal4::RoleStateReceiverNames::kDragInline &&
              InlineText(direct) == "DragReceiver" &&
              runtime.calls == std::vector<std::string>{"grow:12"},
          "drag create returns inline storage");
}

void TestCanReceiver() {
    Runtime runtime;
    pal4::PreviewTitleStringState state;
    Check(pal4::RoleStateReceiverNames::InitializeCan(state, runtime) ==
              23 && state.Read(0u) == 11u &&
              state.Read(4u) == 32u &&
              InlineText(state) == "CanReceiver" &&
              runtime.calls == std::vector<std::string>{
                  "grow:11", "atexit:" + std::to_string(
                      pal4::RoleStateReceiverNames::kCanCleanup)},
          "can name initialization and cleanup registration");
    pal4::RoleStateReceiverNames::DestroyCan(state, runtime);
    Check(runtime.calls.back() == "destroy", "can name destructor");

    runtime.use_heap = true;
    runtime.calls.clear();
    pal4::PreviewTitleStringState heap_state;
    Check(pal4::RoleStateReceiverNames::CreateCan(heap_state, runtime) ==
              0x9000u && heap_state.Read(0u) == 11u &&
              runtime.heap[0x9000u] == 'C' &&
              runtime.heap[0x9000u + 11u * 4u] == 0u,
          "can name writes codepoints to grown heap storage");
}

}  // namespace

int main() {
    TestDragReceiver();
    TestCanReceiver();
}
