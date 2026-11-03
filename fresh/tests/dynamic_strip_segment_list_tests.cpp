#include "pal4/dynamic_strip_object_update.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IDynamicStripSegmentListRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::DynamicStripSegment> segments;
    std::vector<std::string> calls;
    std::uint32_t next_node = 0x800u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 36u, "node allocation size");
        calls.push_back("allocate");
        const auto node = next_node;
        next_node += 0x40u;
        return node;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void CopySegment(std::uint32_t address,
                     const pal4::DynamicStripSegment& segment) override {
        calls.push_back("copy:" + std::to_string(address));
        segments.emplace(address, segment);
    }
};

pal4::DynamicStripSegment Segment(float value) {
    return {{value, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, 7.0f};
}

void CircularList() {
    Runtime runtime;
    runtime.words[0x704u] = 0x700u;
    runtime.words[0x700u] = 0x700u;
    std::uint32_t count = 0u;
    const auto first = pal4::DynamicStripSegmentList::Append(
        0x700u, count, Segment(1.0f), runtime);
    Check(first == 0x800u && count == 1u &&
              runtime.words[0x700u] == first &&
              runtime.words[0x704u] == first &&
              runtime.words[first] == 0x700u &&
              runtime.words[first + 4u] == 0x700u,
          "first node links into empty circular list");
    Check(runtime.segments.at(first + 8u).first[0] == 1.0f,
          "first 28-byte payload copied");

    const auto second = pal4::DynamicStripSegmentList::Append(
        0x700u, count, Segment(9.0f), runtime);
    Check(second == 0x840u && count == 2u &&
              runtime.words[0x700u] == first &&
              runtime.words[first] == second &&
              runtime.words[second] == 0x700u &&
              runtime.words[second + 4u] == first &&
              runtime.words[0x704u] == second,
          "second node appends after old tail");
    Check(runtime.segments.at(second + 8u).first[0] == 9.0f,
          "second payload copied independently");
    Check(runtime.calls[0] == "read:1796" &&
              runtime.calls[1] == "allocate" &&
              runtime.calls[2] == "write:2048:1792" &&
              runtime.calls[6] == "copy:2056",
          "native read, allocation, link and copy order");
}

void NullPredecessor() {
    Runtime runtime;
    runtime.words[0x704u] = 0u;
    std::uint32_t count = 10u;
    const auto node = pal4::DynamicStripSegmentList::Append(
        0x700u, count, Segment(3.0f), runtime);
    Check(count == 11u && runtime.words[node] == node &&
              runtime.words[node + 4u] == node &&
              runtime.words[0x704u] == node,
          "native null predecessor fallback points node to itself");
}

}  // namespace

int main() {
    CircularList();
    NullPredecessor();
}
