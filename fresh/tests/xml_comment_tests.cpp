#include "pal4/xml_comment.h"

#include "pal4/compact_string_buffer.h"
#include "pal4/xml_parser_node.h"
#include "pal4/xml_serializer.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Parser final : pal4::IXmlParserNodeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    std::uint32_t Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return 0u;
    }
    std::uint32_t DeleteNode(std::uint32_t, std::uint32_t,
                             std::uint32_t) override { return 0u; }
    std::uint32_t StringLength(const std::uint32_t address) override {
        calls.push_back("length:" + std::to_string(address));
        return 4u;
    }
    std::uint32_t AssignString(const std::uint32_t destination,
                               const std::uint32_t source,
                               const std::uint32_t length) override {
        calls.push_back("assign:" + std::to_string(destination) + ":" +
                        std::to_string(source) + ":" +
                        std::to_string(length));
        return destination;
    }
    std::int32_t CompareText(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    std::uint32_t CallVirtual(std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    std::uint32_t AttributeSetup(std::uint32_t, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 std::uint32_t) override { return 0u; }
};

struct CommentRuntime final : pal4::IXmlLeafRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation{0x3000u};
    std::uint32_t visitor_result{0x78u};

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t CallVisitor(const std::uint32_t visitor,
                              const std::uint32_t comment,
                              const std::uint32_t slot) override {
        calls.push_back("visit:" + std::to_string(visitor) + ":" +
                        std::to_string(comment) + ":" +
                        std::to_string(slot));
        return visitor_result;
    }
};

struct Serialization final : pal4::IXmlSerializationRuntime {
    std::uint32_t block{0x5000u};
    std::string text{"memo"};
    std::uint32_t ReadWord(std::uint32_t) override { return block; }
    std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    std::string ReadText(std::uint32_t) override { return text; }
    std::uint32_t StoreTemporary(std::string_view) override { return 0u; }
    std::uint32_t FreeTemporary(std::uint32_t) override { return 0u; }
    int WriteVirtual(std::uint32_t, std::FILE*, int,
                     std::uint32_t) override { return 0; }
    std::uint32_t NextSibling(std::uint32_t) override { return 0u; }
    bool IsInline(std::uint32_t, std::uint32_t) override { return false; }
};

std::string ReadAll(std::FILE* stream) {
    std::fflush(stream);
    std::rewind(stream);
    std::string result;
    int character;
    while ((character = std::fgetc(stream)) != EOF)
        result.push_back(static_cast<char>(character));
    return result;
}

void TestCreateAndCleanup() {
    Parser parser;
    CommentRuntime runtime;
    parser.words[0x2020u] = 0x6000u;
    parser.words[0x200cu] = 7u;
    Check(pal4::XmlComment::Create(0x2000u, parser, runtime) == 0x3000u &&
              runtime.calls == std::vector<std::string>{"allocate:44"} &&
              parser.words[0x3000u] == pal4::XmlComment::kVtable &&
              parser.words[0x3014u] == 2u &&
              parser.words[0x3020u] ==
                  pal4::CompactStringBuffer::kEmptyBlock &&
              parser.words[0x300cu] == 7u &&
              parser.calls[parser.calls.size() - 2u] ==
                  "assign:12320:24584:4" &&
              parser.calls.back() == "write:12300:7",
          "create initializes a 44-byte comment then copies source text");
    parser.calls.clear();
    pal4::XmlComment::Cleanup(0x3000u, parser);
    Check(parser.words[0x3000u] == 0x8499b4u &&
              parser.calls.front() == "write:12288:8690264" &&
              parser.calls[1] == "write:12288:8690032",
          "cleanup installs comment vtable before parser/base cleanup");
    runtime.allocation = 0u;
    parser.calls.clear();
    Check(pal4::XmlComment::Create(0x2000u, parser, runtime) == 0u &&
              parser.calls.empty(),
          "allocation failure leaves parser untouched");
}

void TestWriteAndVisitor() {
    Serialization serialization;
    std::unique_ptr<std::FILE, decltype(&std::fclose)> stream(
        std::tmpfile(), &std::fclose);
    Check(stream != nullptr, "temporary stream opens");
    Check(pal4::XmlComment::WriteToFile(
              0x3000u, stream.get(), 2, serialization) == 11 &&
              ReadAll(stream.get()) == "        <!--memo-->",
          "comment writer emits four-space indentation and comment text");
    CommentRuntime runtime;
    Check(pal4::XmlComment::IsComment(0x3000u, 0x7000u, runtime) ==
              0x78u &&
              runtime.calls ==
                  std::vector<std::string>{"visit:28672:12288:24"},
          "comment visitor dispatches slot 0x18 with comment argument");
    pal4::XmlLeafRuntimeStub stub;
    Check(stub.Allocate(44u) == 0u &&
              stub.CallVisitor(0u, 0u, 24u) == 0u,
          "comment native boundary has a linkable stub");
}

}  // namespace

int main() {
    TestCreateAndCleanup();
    TestWriteAndVisitor();
}
