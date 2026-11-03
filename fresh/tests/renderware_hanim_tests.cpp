#include "pal4/renderware_hanim.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IRenderWareHAnimRuntime {
public:
    std::map<std::uint32_t, std::vector<std::uint32_t>> children;
    std::map<std::uint32_t, std::uint32_t> hierarchies;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> visited;
    std::vector<std::uint32_t> enumerated;
    std::uint32_t assigned_atomic = 0u;
    std::uint32_t assigned_hierarchy = 0u;
    std::uint32_t id = 0x91u;

    std::uint32_t GetHierarchy(std::uint32_t frame) override {
        visited.push_back(frame);
        return hierarchies[frame];
    }
    void ForAllChildren(std::uint32_t frame,
                        const FrameVisitor& visitor) override {
        enumerated.push_back(frame);
        for (const auto child : children[frame]) {
            if (visitor(child) == 0u) break;
        }
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void SetSkinHierarchy(std::uint32_t atomic,
                          std::uint32_t hierarchy) override {
        assigned_atomic = atomic;
        assigned_hierarchy = hierarchy;
    }
    std::uint32_t GetFrameId(std::uint32_t frame) override {
        Check(frame == 0x40u, "frame id input");
        return id;
    }
};

void RecursiveCallbackFindsAndStops() {
    Runtime runtime;
    runtime.children[0x10u] = {0x20u, 0x30u};
    runtime.children[0x20u] = {0x40u};
    runtime.hierarchies[0x40u] = 0x1234u;
    std::uint32_t result = 0xaaaau;
    Check(pal4::HAnimFrameWalker::Visit(0x10u, result, runtime) == 0x10u &&
              result == 0x1234u &&
              runtime.visited == std::vector<std::uint32_t>{
                  0x10u, 0x20u, 0x40u, 0x30u},
          "callback returns original parent after child traversal");
    Check(runtime.enumerated == std::vector<std::uint32_t>{
              0x10u, 0x20u, 0x30u},
          "nonmatching ancestors keep traversal active");
    runtime.visited.clear();
    runtime.enumerated.clear();
    runtime.hierarchies[0x10u] = 0x5678u;
    Check(pal4::HAnimFrameWalker::Visit(0x10u, result, runtime) == 0u &&
              result == 0x5678u && runtime.visited.size() == 1u &&
              runtime.enumerated.empty(),
          "direct hierarchy match returns zero without visiting children");
}

void AtomicEntryOnlySearchesFrameChildren() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x10u;
    runtime.hierarchies[0x10u] = 0xbeefu;
    runtime.children[0x10u] = {0x20u, 0x30u};
    runtime.hierarchies[0x20u] = 0x2222u;
    Check(pal4::HAnimFrameWalker::FindInAtomicChildren(0u, runtime) == 0u &&
              runtime.enumerated.empty(),
          "null atomic returns before reading its frame");
    Check(pal4::HAnimFrameWalker::FindInAtomicChildren(
              0x1000u, runtime) == 0x2222u &&
              runtime.visited == std::vector<std::uint32_t>{0x20u} &&
              runtime.enumerated == std::vector<std::uint32_t>{0x10u},
          "root is excluded and direct child match stops siblings");
    runtime.hierarchies[0x20u] = 0u;
    runtime.hierarchies[0x30u] = 0u;
    runtime.visited.clear();
    Check(pal4::HAnimFrameWalker::FindInAtomicChildren(
              0x1000u, runtime) == 0u,
          "unmatched children leave output zero");
}

void SkinAndFrameIdCallbacksForwardArguments() {
    Runtime runtime;
    Check(pal4::HAnimFrameWalker::AssignSkinHierarchy(
              0x5000u, 0x6000u, runtime) == 0x5000u &&
              runtime.assigned_atomic == 0x5000u &&
              runtime.assigned_hierarchy == 0x6000u,
          "skin hierarchy callback forwards arguments and returns atomic");
    std::uint32_t id = 0u;
    Check(pal4::HAnimFrameWalker::CaptureFrameId(
              0x40u, id, runtime) == 0u && id == 0x91u,
          "frame id callback stores id and stops enumeration");
    pal4::RenderWareHAnimStub stub;
    Check(pal4::HAnimFrameWalker::FindInAtomicChildren(0u, stub) == 0u &&
              pal4::HAnimFrameWalker::AssignSkinHierarchy(
                  0x77u, 0x88u, stub) == 0x77u,
          "RenderWare-free stub links and runs");
}

}  // namespace

int main() {
    RecursiveCallbackFindsAndStops();
    AtomicEntryOnlySearchesFrameChildren();
    SkinAndFrameIdCallbacksForwardArguments();
    std::cout << "3 RenderWare HAnim test cases passed\n";
}
