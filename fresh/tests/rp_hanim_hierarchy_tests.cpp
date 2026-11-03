#include "pal4/rp_hanim_hierarchy.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IRpHAnimHierarchyRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> hierarchies;
    std::map<std::uint32_t, std::vector<std::uint32_t>> children;
    std::uint32_t root = 0x100u;
    std::uint32_t atomic_result = 0;
    std::vector<std::string> calls;

    std::uint32_t FrameHierarchy(std::uint32_t frame) override {
        calls.push_back("hierarchy:" + std::to_string(frame));
        return hierarchies[frame];
    }
    void ForAllChildren(std::uint32_t frame,
                        pal4::RpHAnimFrameCallback callback,
                        std::uint32_t& found) override {
        calls.push_back("children:" + std::to_string(frame));
        for (const auto child : children[frame]) {
            if (callback(child, found, *this) == 0) break;
        }
    }
    std::uint32_t ClumpFrame(std::uint32_t clump) override {
        calls.push_back("clump:" + std::to_string(clump));
        return root;
    }
    void ForAllAtomicsGetId(std::uint32_t clump,
                            std::uint32_t& found) override {
        calls.push_back("atomics:" + std::to_string(clump));
        found = atomic_result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    std::uint32_t found = 0x999u;
    runtime.hierarchies[0x200u] = 0x700u;
    Check(pal4::RpHAnimHierarchy::FindInFrame(
              0x200u, found, runtime) == 0 &&
              found == 0x700u &&
              runtime.calls == std::vector<std::string>{"hierarchy:512"},
          "direct hierarchy writes output and stops child traversal");

    runtime.calls.clear();
    found = 0;
    runtime.children[0x200u] = {0x300u, 0x400u};
    runtime.children[0x300u] = {0x500u};
    runtime.hierarchies[0x200u] = 0;
    runtime.hierarchies[0x500u] = 0x800u;
    Check(pal4::RpHAnimHierarchy::FindInFrame(
              0x200u, found, runtime) == 0x200u &&
              found == 0x800u &&
              runtime.calls == std::vector<std::string>{
                  "hierarchy:512", "children:512", "hierarchy:768",
                  "children:768", "hierarchy:1280", "hierarchy:1024",
                  "children:1024"},
          "nested hierarchy sets output while parent callback returns frame");

    runtime.calls.clear();
    Check(pal4::RpHAnimHierarchy::FindInClump(0u, runtime) == 0u &&
              runtime.calls.empty(),
          "null clump returns zero without RenderWare calls");
    runtime.calls.clear();
    runtime.children[0x100u] = {0x200u};
    Check(pal4::RpHAnimHierarchy::FindInClump(
              0x600u, runtime) == 0x800u &&
              runtime.calls.front() == "clump:1536" &&
              runtime.calls[1] == "children:256",
          "clump wrapper starts with children of clump frame");

    runtime.calls.clear();
    runtime.atomic_result = 0;
    Check(!pal4::RpHAnimHierarchy::HasHierarchy(0u, runtime) &&
              runtime.calls == std::vector<std::string>{"atomics:0"},
          "has-hierarchy forwards null clump to atomic traversal");
    runtime.calls.clear();
    runtime.atomic_result = 0x1234u;
    Check(pal4::RpHAnimHierarchy::HasHierarchy(0x600u, runtime) &&
              runtime.calls == std::vector<std::string>{"atomics:1536"},
          "any nonzero atomic result means hierarchy present");

    std::uint32_t flags = 0x80u;
    Check(pal4::RpHAnimHierarchy::SetFlags(flags, 0x05u) == &flags &&
              flags == 0x85u,
          "set-flags returns original word and ORs mask");

    pal4::RpHAnimHierarchyRuntimeStub stub;
    Check(pal4::RpHAnimHierarchy::FindInClump(0x100u, stub) == 0u,
          "RenderWare stub is linkable");
    std::cout << "rp_hanim_hierarchy_tests passed\n";
}
