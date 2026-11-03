#include "pal4/xml_text_node.h"

#include "pal4/compact_string_buffer.h"
#include "pal4/xml_parser_document.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::XmlDocumentRuntimeStub {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t next{0x3000u};
    bool fail_initial_assignment{};

    std::uint32_t Allocate(const std::uint32_t size) override {
        calls.push_back("allocate:" + std::to_string(size));
        return next;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        calls.push_back("byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    std::uint32_t StringLength(const std::uint32_t source) override {
        calls.push_back("length:" + std::to_string(source));
        return source == pal4::XmlTextNode::kInitialText ? 0u : 4u;
    }
    std::uint32_t AssignString(const std::uint32_t destination,
                               const std::uint32_t source,
                               const std::uint32_t length) override {
        calls.push_back("assign:" + std::to_string(destination) + ":" +
                        std::to_string(source) + ":" +
                        std::to_string(length));
        if (fail_initial_assignment &&
            source == pal4::XmlTextNode::kInitialText)
            throw std::runtime_error("initial assignment");
        words[destination] = source == pal4::XmlTextNode::kInitialText ?
            pal4::CompactStringBuffer::kEmptyBlock : 0x6000u;
        return destination;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address;
    }
};

struct Visitor final : pal4::IXmlLeafRuntime {
    std::vector<std::string> calls;
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t CallVisitor(const std::uint32_t visitor,
                              const std::uint32_t node,
                              const std::uint32_t slot) override {
        calls.push_back("visit:" + std::to_string(visitor) + ":" +
                        std::to_string(node) + ":" +
                        std::to_string(slot));
        return 0x42u;
    }
};

void TestCreateAndCleanup() {
    Runtime runtime;
    runtime.words[0x1020u] = 0x5000u;
    runtime.words[0x100cu] = 7u;
    runtime.bytes[0x102cu] = 1u;
    Check(pal4::XmlTextNode::Create(0x1000u, runtime) == 0x3000u &&
              runtime.calls.front() == "allocate:48" &&
              runtime.words[0x3000u] == pal4::XmlTextNode::kVtable &&
              runtime.words[0x3014u] == 4u &&
              runtime.words[0x300cu] == 7u &&
              runtime.words[0x3020u] == 0x6000u &&
              runtime.bytes[0x302cu] == 1u,
          "create allocates 48 bytes, initializes kind four and copies source");
    const auto initial = "assign:12320:" +
        std::to_string(pal4::XmlTextNode::kInitialText) + ":0";
    const auto copied = std::string{"assign:12320:20488:4"};
    auto initial_index = runtime.calls.size();
    auto copy_index = runtime.calls.size();
    for (std::size_t i = 0; i < runtime.calls.size(); ++i) {
        if (runtime.calls[i] == initial) initial_index = i;
        if (runtime.calls[i] == copied) copy_index = i;
    }
    Check(initial_index < copy_index && copy_index < runtime.calls.size(),
          "initial text assignment precedes source text copy");
    runtime.calls.clear();
    pal4::XmlTextNode::Cleanup(0x3000u, runtime);
    Check(runtime.words[0x3000u] == 0x8499b4u &&
              runtime.calls.front() == "write:12288:8690336" &&
              runtime.calls.back() == "write:12288:8690100",
          "cleanup restores parser and base vtables after text block release");
    bool freed_text = false;
    for (const auto& call : runtime.calls)
        if (call == "free:24576") freed_text = true;
    Check(freed_text, "cleanup frees copied compact text block");
}

void TestFailureAndVisitor() {
    Runtime runtime;
    runtime.next = 0u;
    Check(pal4::XmlTextNode::Create(0x1000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:48"},
          "allocation failure performs no parser writes");
    runtime.next = 0x3000u;
    runtime.fail_initial_assignment = true;
    runtime.calls.clear();
    bool caught = false;
    try {
        (void)pal4::XmlTextNode::Create(0x1000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "free:12288" &&
              runtime.words[0x3000u] == 0x8499b4u,
          "initial assignment failure cleans parser and allocation");
    Visitor visitor;
    Check(pal4::XmlTextNode::AcceptVisitor(
              0x3000u, 0x7000u, visitor) == 0x42u &&
              visitor.calls ==
                  std::vector<std::string>{"visit:28672:12288:28"},
          "text visitor uses slot 0x1c");
}

void TestWhitespaceOnly() {
    Runtime runtime;
    runtime.words[0x3020u] = 0x6000u;
    runtime.words[0x6000u] = 0u;
    Check(pal4::XmlTextNode::IsWhitespaceOnly(0x3000u, runtime),
          "empty text is whitespace only");
    runtime.words[0x6000u] = 4u;
    runtime.bytes[0x6008u] = ' ';
    runtime.bytes[0x6009u] = '\t';
    runtime.bytes[0x600au] = '\r';
    runtime.bytes[0x600bu] = '\n';
    Check(pal4::XmlTextNode::IsWhitespaceOnly(0x3000u, runtime),
          "spaces, tabs and both newline bytes are accepted");
    runtime.bytes[0x600au] = 'x';
    Check(!pal4::XmlTextNode::IsWhitespaceOnly(0x3000u, runtime),
          "non-whitespace byte rejects text");
    runtime.bytes[0x600au] = 0u;
    Check(!pal4::XmlTextNode::IsWhitespaceOnly(0x3000u, runtime),
          "stored length controls scan even across an embedded NUL");
}

}  // namespace

int main() {
    TestCreateAndCleanup();
    TestFailureAndVisitor();
    TestWhitespaceOnly();
}
