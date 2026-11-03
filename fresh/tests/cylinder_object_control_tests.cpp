#include "pal4/cylinder_object_control.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICylinderObjectControlRuntime {
    std::vector<std::string> calls;

    void StopFacet(pal4::CylinderObjectState& state,
                   std::uint32_t offset) override {
        Check((state.words[1u] & 0xffu) != 0u,
              "facets stop before activity clears");
        calls.push_back("facet:" + std::to_string(offset));
    }
    void DestroyModel(std::uint32_t model) override {
        Check(model == 0x2000u, "visual model destruction target");
        calls.push_back("destroy_model");
    }
    void FreeModel(std::uint32_t model) override {
        Check(model == 0x2000u, "visual model free target");
        calls.push_back("free_model");
    }
    void DestroyDocument(std::uint32_t document) override {
        Check(document == 0x3000u, "XML destruction target");
        calls.push_back("destroy_document");
    }
    void FreeDocument(std::uint32_t document) override {
        Check(document == 0x3000u, "XML free target");
        calls.push_back("free_document");
    }
    void ProcessEvent(std::uint32_t system,
                      pal4::CylinderObjectState& state) override {
        Check(system == 0x5000u && (state.words[1u] & 0xffu) == 0u,
              "owner event after activity clear");
        calls.push_back("event");
    }
    std::uint32_t DeleteSelf(pal4::CylinderObjectState& state,
                              std::uint32_t flags) override {
        Check(flags == 1u && (state.words[1u] & 0xffu) == 0u,
              "unconditional deleting destructor flag");
        calls.push_back("delete_self");
        return 0xabcdef01u;
    }
};

void TestActive() {
    pal4::CylinderObjectState state{};
    state.words.fill(0xa5a5a5a5u);
    state.words[1u] = 0x11223301u;
    state.words[4u] = 0x5000u;
    state.words[22u] = 0x2000u;
    state.words[23u] = 0x3000u;
    Runtime runtime;
    Check(pal4::CylinderObjectControl::StopAndDestroy(
              state, runtime) == 0xabcdef01u &&
              runtime.calls == std::vector<std::string>{
                  "facet:48", "facet:20", "destroy_model", "free_model",
                  "destroy_document", "free_document", "event",
                  "delete_self"},
          "active stop follows native facet, resource and owner order");
    Check(state.words[1u] == 0x00003300u &&
              state.words[15u] == 0u &&
              state.words[16u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[17u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[18u] == 0u && state.words[19u] == 0u &&
              state.words[20u] == 0u &&
              (state.words[21u] & 0xffu) == 0u &&
              state.words[22u] == 0u && state.words[23u] == 0u,
          "active stop clears exact fields and preserves byte +5");
}

void TestInactiveAndEmptyResources() {
    pal4::CylinderObjectState state{};
    state.words[1u] = 0x12340000u;
    state.words[22u] = 0x2000u;
    Runtime runtime;
    Check(pal4::CylinderObjectControl::StopAndDestroy(
              state, runtime) == 0xabcdef01u &&
              runtime.calls == std::vector<std::string>{"delete_self"} &&
              state.words[22u] == 0x2000u,
          "inactive stop delegates directly to deleting destructor");
    state.words[1u] = 1u;
    state.words[22u] = 0u;
    state.words[4u] = 0x5000u;
    runtime.calls.clear();
    Check(pal4::CylinderObjectControl::StopAndDestroy(
              state, runtime) == 0xabcdef01u &&
              runtime.calls == std::vector<std::string>{
                  "facet:48", "facet:20", "event", "delete_self"},
          "active stop skips absent model and XML");
}

}  // namespace

int main() {
    TestActive();
    TestInactiveAndEmptyResources();
}
