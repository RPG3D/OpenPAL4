#include "pal4/xml_node_array.h"

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

struct Runtime final : pal4::IXmlNodeArrayRuntime {
    pal4::XmlNodeArrayState* state = nullptr;
    std::vector<std::string> calls;
    std::map<std::uint32_t, int> nodes;
    std::uint32_t nextAllocation = 0x2000u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("alloc:" + std::to_string(bytes));
        return nextAllocation;
    }
    std::uint32_t CopyConstruct(std::uint32_t destination,
                                std::uint32_t source) override {
        calls.push_back("construct:" + std::to_string(destination) + ":" +
                        std::to_string(source));
        nodes[destination] = nodes.at(source);
        return destination;
    }
    std::uint32_t CopyAssign(std::uint32_t destination,
                             std::uint32_t source) override {
        calls.push_back("assign:" + std::to_string(destination) + ":" +
                        std::to_string(source));
        nodes[destination] = nodes.at(source);
        return destination;
    }

    void DestroyNode(std::uint32_t address) override {
        calls.push_back("destroy:" + std::to_string(address));
        Check(state->words[1] != 0u && state->words[2] != 0u,
              "node array remains intact during element destruction");
        nodes.erase(address);
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        Check(state->words[1] == address,
              "array pointers clear after free call");
    }
};

void TestExpansion() {
    pal4::XmlNodeArrayState state{{0x55u, 0x1000u, 0x10b4u, 0x10b4u}};
    Runtime runtime;
    runtime.state = &state;
    runtime.nodes = {{0x1000u, 1}, {0x103cu, 2}, {0x1078u, 3}, {0x3000u, 9}};
    Check(pal4::XmlNodeArray::InsertCopies(state, 0x103cu, 2u, 0x3000u,
                                           runtime) == 15u,
          "expansion returns three times new element count");
    Check(state.words == std::array<std::uint32_t, 4>{
              0x55u, 0x2000u, 0x212cu, 0x2168u},
          "expansion updates begin, end and doubled capacity");
    Check(runtime.nodes.at(0x2000u) == 1 &&
              runtime.nodes.at(0x203cu) == 9 &&
              runtime.nodes.at(0x2078u) == 9 &&
              runtime.nodes.at(0x20b4u) == 2 &&
              runtime.nodes.at(0x20f0u) == 3,
          "prefix, inserted copies and suffix retain order");
    Check(runtime.calls == std::vector<std::string>{
              "alloc:360", "construct:8192:4096", "construct:8252:12288",
              "construct:8312:12288", "construct:8372:4156",
              "construct:8432:4216", "destroy:4096", "destroy:4156",
              "destroy:4216", "free:4096"},
          "expansion constructs before destroying and freeing old nodes");
}

void TestEmptyExpansion() {
    pal4::XmlNodeArrayState state{{7u, 0u, 0u, 0u}};
    Runtime runtime;
    runtime.state = &state;
    runtime.nodes[0x3000u] = 4;
    Check(pal4::XmlNodeArray::InsertCopies(state, 0u, 2u, 0x3000u,
                                           runtime) == 6u &&
              state.words == std::array<std::uint32_t, 4>{
                  7u, 0x2000u, 0x2078u, 0x2078u} &&
              runtime.calls == std::vector<std::string>{
                  "alloc:120", "construct:8192:12288",
                  "construct:8252:12288", "free:0"},
          "empty array allocates the requested count and frees null begin");
}

void TestShortTail() {
    pal4::XmlNodeArrayState state{{3u, 0x1000u, 0x1078u, 0x1200u}};
    Runtime runtime;
    runtime.state = &state;
    runtime.nodes = {{0x1000u, 1}, {0x103cu, 2}, {0x3000u, 9}};
    Check(pal4::XmlNodeArray::InsertCopies(state, 0x103cu, 3u, 0x3000u,
                                           runtime) == 0x112cu &&
              state.words[2] == 0x112cu &&
              runtime.nodes.at(0x1000u) == 1 &&
              runtime.nodes.at(0x103cu) == 9 &&
              runtime.nodes.at(0x1078u) == 9 &&
              runtime.nodes.at(0x10b4u) == 9 &&
              runtime.nodes.at(0x10f0u) == 2,
          "short-tail branch constructs shifted tail and extra copies");
    Check(runtime.calls == std::vector<std::string>{
              "construct:4336:4156", "construct:4216:12288",
              "construct:4276:12288", "assign:4156:12288"},
          "short-tail branch uses construction before original-slot assignment");
}

void TestLongTail() {
    pal4::XmlNodeArrayState state{{3u, 0x1000u, 0x10f0u, 0x1200u}};
    Runtime runtime;
    runtime.state = &state;
    runtime.nodes = {{0x1000u, 1}, {0x103cu, 2}, {0x1078u, 3},
                     {0x10b4u, 4}, {0x3000u, 9}};
    Check(pal4::XmlNodeArray::InsertCopies(state, 0x103cu, 2u, 0x3000u,
                                           runtime) == 0x1078u &&
              state.words[2] == 0x1168u &&
              runtime.nodes.at(0x1000u) == 1 &&
              runtime.nodes.at(0x103cu) == 9 &&
              runtime.nodes.at(0x1078u) == 9 &&
              runtime.nodes.at(0x10b4u) == 2 &&
              runtime.nodes.at(0x10f0u) == 3 &&
              runtime.nodes.at(0x112cu) == 4,
          "long-tail branch shifts backwards and returns last assignment result");
    Check(runtime.calls == std::vector<std::string>{
              "construct:4336:4216", "construct:4396:4276",
              "assign:4276:4156", "assign:4156:12288",
              "assign:4216:12288"},
          "long-tail branch keeps the assembly's construction and assignment order");
}

void TestTailAndZero() {
    pal4::XmlNodeArrayState state{{3u, 0x1000u, 0x103cu, 0x1100u}};
    Runtime runtime;
    runtime.state = &state;
    runtime.nodes = {{0x1000u, 1}, {0x3000u, 9}};
    Check(pal4::XmlNodeArray::InsertCopies(state, 0x103cu, 1u, 0x3000u,
                                           runtime) == 0x1078u &&
              runtime.calls == std::vector<std::string>{
                  "construct:4156:12288"},
          "tail insertion constructs directly into spare capacity");
    runtime.calls.clear();
    Check(pal4::XmlNodeArray::InsertCopies(state, 0x1000u, 0u, 0x3000u,
                                           runtime) == 0u &&
              state.words[2] == 0x1078u && runtime.calls.empty(),
          "zero-count insertion leaves nodes untouched");
    Check(pal4::XmlNodeArray::CopyConstructNode(
              0u, 0x3000u, runtime, 0xabcdu) == 0xabcdu &&
              pal4::XmlNodeArray::CopyConstructNode(
                  0x4000u, 0x3000u, runtime) == 0x4000u &&
              runtime.nodes.at(0x4000u) == 9,
          "copy-construction wrapper preserves undefined EAX on null destination");
}

}  // namespace

int main() {
    TestExpansion();
    TestEmptyExpansion();
    TestShortTail();
    TestLongTail();
    TestTailAndZero();
    pal4::XmlNodeArrayState state;
    state.words = {0x55u, 0x1000u, 0x1078u, 0x1100u};
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::XmlNodeArray::Cleanup(state, runtime) == 0u &&
              state.words == std::array<std::uint32_t, 4>{0x55u, 0u, 0u, 0u} &&
              runtime.calls == std::vector<std::string>{
                  "destroy:4096", "destroy:4156", "free:4096"},
          "two XML nodes are destroyed at 60-byte stride before free");
    runtime.calls.clear();
    Check(pal4::XmlNodeArray::Cleanup(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"free:0"},
          "empty array still calls free on zero");
    pal4::XmlNodeArrayRuntimeStub stub;
    Check(pal4::XmlNodeArray::Cleanup(state, stub) == 0u,
          "default runtime links");
    std::cout << "7 xml node array cases passed\n";
}
