#include "pal4/xml_buffer_release.h"

#include "pal4/compact_string_buffer.h"
#include "pal4/xml_parser_document.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IXmlParserNodeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> freed;

    std::uint32_t ReadWord(std::uint32_t address) override { return words[address]; }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        return address + 1u;
    }
    std::uint32_t DeleteNode(std::uint32_t node, std::uint32_t,
                             std::uint32_t) override { return node; }
    std::uint32_t StringLength(std::uint32_t) override { return 0u; }
    std::uint32_t AssignString(std::uint32_t destination, std::uint32_t,
                               std::uint32_t) override { return destination; }
    std::int32_t CompareText(std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t CallVirtual(std::uint32_t, std::uint32_t) override { return 0u; }
    std::uint32_t AttributeSetup(std::uint32_t, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 std::uint32_t) override { return 0u; }
};

void TestSentinelAndHeapBlock() {
    Runtime runtime;
    runtime.words[0x3000u] = pal4::CompactStringBuffer::kEmptyBlock;
    Check(pal4::XmlBufferRelease::Release(0x3000u, runtime) ==
              pal4::CompactStringBuffer::kEmptyBlock &&
              runtime.freed.empty(),
          "shared empty block is returned without freeing");
    runtime.words[0x3000u] = 0x5000u;
    Check(pal4::XmlBufferRelease::Release(0x3000u, runtime) == 0x5001u &&
              runtime.freed == std::vector<std::uint32_t>{0x5000u},
          "allocated block forwards runtime free result");
}

void TestAttributeCleanupComposition() {
    Runtime runtime;
    runtime.words[0x3014u] = 0x7000u;
    runtime.words[0x3018u] = 0x6000u;
    Check(pal4::XmlAttributeLifecycle::Cleanup(0x3000u, runtime) ==
              0x7001u &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x6000u, 0x7000u} &&
              runtime.words[0x3000u] == 0x8499b4u,
          "attribute cleanup releases value before name");
}

}  // namespace

int main() {
    TestSentinelAndHeapBlock();
    TestAttributeCleanupComposition();
}
