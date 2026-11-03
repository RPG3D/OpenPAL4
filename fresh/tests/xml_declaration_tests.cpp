#include "pal4/xml_declaration.h"

#include "pal4/compact_string_buffer.h"
#include "pal4/xml_leaf_runtime.h"
#include "pal4/xml_parser_document.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
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

struct Parser final : pal4::XmlDocumentRuntimeStub {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation{0x3000u};
    std::uint32_t next_block{0x7000u};
    std::uint32_t fail_on_destination{};
    std::uint32_t Allocate(const std::uint32_t size) override {
        calls.push_back("allocate:" + std::to_string(size));
        return allocation;
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
    std::uint32_t Free(const std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address;
    }
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
        if (destination == fail_on_destination)
            throw std::runtime_error("assignment");
        words[destination] = next_block;
        next_block += 0x100u;
        return destination;
    }
};

struct Visitor final : pal4::IXmlLeafRuntime {
    std::vector<std::uint32_t> call;
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t CallVisitor(const std::uint32_t visitor,
                              const std::uint32_t node,
                              const std::uint32_t slot) override {
        call = {visitor, node, slot};
        return 0x66u;
    }
};

struct Output final : pal4::IXmlDeclarationRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> texts;
    std::vector<std::string> pieces;
    std::vector<std::string> calls;
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::string ReadText(const std::uint32_t address,
                         const std::uint32_t length) override {
        calls.push_back("text:" + std::to_string(address) + ":" +
                        std::to_string(length));
        return texts.at(address).substr(0u, length);
    }
    std::uint32_t Append(const std::uint32_t output,
                         const std::string_view text) override {
        Check(output == 0x9000u, "append target is forwarded");
        pieces.emplace_back(text);
        return output;
    }
    std::string Joined() const {
        std::string result;
        for (const auto& piece : pieces) result += piece;
        return result;
    }
};

using File = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

File OpenFile() {
    File file(std::tmpfile(), &std::fclose);
    Check(file != nullptr, "temporary output file opens");
    return file;
}

std::string ReadAll(std::FILE* file) {
    std::fflush(file);
    std::rewind(file);
    std::string text;
    int character;
    while ((character = std::fgetc(file)) != EOF)
        text.push_back(static_cast<char>(character));
    return text;
}

Output MakeFields() {
    Output output;
    output.words[0x102cu] = 0x5000u;
    output.words[0x1030u] = 0x5100u;
    output.words[0x1034u] = 0x5200u;
    output.words[0x5000u] = 3u;
    output.words[0x5100u] = 5u;
    output.words[0x5200u] = 3u;
    output.texts[0x5008u] = "1.0";
    output.texts[0x5108u] = "UTF-8";
    output.texts[0x5208u] = "yes";
    return output;
}

void TestCleanup() {
    Parser parser;
    parser.words[0x1034u] = 0x6000u;
    parser.words[0x1030u] = pal4::CompactStringBuffer::kEmptyBlock;
    parser.words[0x102cu] = 0x5000u;
    parser.words[0x1018u] = 0u;
    parser.words[0x1020u] = 0x7000u;
    pal4::XmlDeclaration::Cleanup(0x1000u, parser);
    Check(parser.words[0x1000u] == 0x8499b4u &&
              parser.calls.front() == "write:4096:8690408" &&
              parser.calls.back() == "write:4096:8690100",
          "declaration cleanup restores vtables around parser cleanup");
    std::vector<std::string> frees;
    for (const auto& call : parser.calls)
        if (call.starts_with("free:")) frees.push_back(call);
    Check(frees == std::vector<std::string>{
              "free:24576", "free:20480", "free:28672"},
          "declaration fields free in reverse order, then base text");
}

void TestWriteBothOutputs() {
    auto output = MakeFields();
    auto file = OpenFile();
    const std::string expected =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>";
    Check(pal4::XmlDeclaration::Write(
              0x1000u, file.get(), 3, 0x9000u, output) == 0x9000 &&
              ReadAll(file.get()) == expected && output.Joined() == expected &&
              output.pieces == std::vector<std::string>{
                  "<?xml ", "version=\"", "1.0", "\" ",
                  "encoding=\"", "UTF-8", "\" ",
                  "standalone=\"", "yes", "\" ", "?>"},
          "declaration writes file and compact output in native piece order");
    output.pieces.clear();
    Check(pal4::XmlDeclaration::Write(
              0x1000u, nullptr, 0, 0x9000u, output) == 0x9000 &&
              output.Joined() == expected,
          "string-only declaration output is complete");
}

void TestEmptyAndFileOnly() {
    auto output = MakeFields();
    auto file = OpenFile();
    Check(pal4::XmlDeclaration::Write(
              0x1000u, file.get(), 0, 0u, output) == 2 &&
              ReadAll(file.get()) ==
                  "<?xml version=\"1.0\" encoding=\"UTF-8\" "
                  "standalone=\"yes\" ?>" && output.pieces.empty(),
          "file-only declaration returns final print count");
    output.words[0x5000u] = 0u;
    output.words[0x5100u] = 0u;
    output.words[0x5200u] = 0u;
    file = OpenFile();
    output.calls.clear();
    Check(pal4::XmlDeclaration::Write(
              0x1000u, file.get(), 0, 0u, output) == 2 &&
              ReadAll(file.get()) == "<?xml ?>" &&
              output.calls.size() == 6u,
          "empty fields are skipped after their length checks");
    Check(pal4::XmlDeclaration::Write(
              0x1000u, nullptr, 0, 0u, output) == 0x5200,
          "no-output path preserves final field block return");
    pal4::XmlDeclarationRuntimeStub stub;
    Check(stub.ReadWord(0u) == 0u && stub.ReadText(0u, 0u).empty() &&
              stub.Append(0u, {}) == 0u,
          "declaration native boundary has a linkable stub");
}

void TestCloneCreateAndVisitor() {
    Parser parser;
    parser.words[0x1020u] = 0x6000u;
    parser.words[0x100cu] = 9u;
    parser.words[0x102cu] = 0x5000u;
    parser.words[0x1030u] = 0x5100u;
    parser.words[0x1034u] = 0x5200u;
    parser.words[0x5000u] = 3u;
    parser.words[0x5100u] = 5u;
    parser.words[0x5200u] = 3u;
    Check(pal4::XmlDeclaration::CloneInto(
              0x1000u, 0x3000u, parser) == 0x3034u &&
              parser.words[0x300cu] == 9u &&
              parser.calls[2] == "assign:12320:24584:4",
          "clone copies base text and field before declaration strings");
    std::vector<std::string> assignments;
    for (const auto& call : parser.calls)
        if (call.starts_with("assign:")) assignments.push_back(call);
    Check(assignments == std::vector<std::string>{
              "assign:12320:24584:4", "assign:12332:20488:3",
              "assign:12336:20744:5", "assign:12340:21000:3"},
          "clone assigns version, encoding and standalone in order");

    parser.calls.clear();
    parser.next_block = 0x7000u;
    Check(pal4::XmlDeclaration::Create(0x1000u, parser) == 0x3000u &&
              parser.calls.front() == "allocate:56" &&
              parser.words[0x3000u] == pal4::XmlDeclaration::kVtable &&
              parser.words[0x3014u] == 5u &&
              parser.words[0x3034u] == 0x7300u,
          "create initializes 56-byte kind-five declaration then clones");
    parser.allocation = 0u;
    parser.calls.clear();
    Check(pal4::XmlDeclaration::Create(0x1000u, parser) == 0u &&
              parser.calls == std::vector<std::string>{"allocate:56"},
          "allocation failure leaves declaration untouched");

    parser.allocation = 0x3000u;
    parser.fail_on_destination = 0x3030u;
    parser.calls.clear();
    bool caught = false;
    try {
        (void)pal4::XmlDeclaration::Create(0x1000u, parser);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && parser.calls.back() == "free:12288" &&
              parser.words[0x3000u] == 0x8499b4u,
          "clone failure cleans partial declaration and allocation");

    Visitor visitor;
    Check(pal4::XmlDeclaration::AcceptVisitor(
              0x3000u, 0x9000u, visitor) == 0x66u &&
              visitor.call == std::vector<std::uint32_t>{
                  0x9000u, 0x3000u, 32u},
          "declaration visitor uses slot 0x20");
}

}  // namespace

int main() {
    TestCleanup();
    TestWriteBothOutputs();
    TestEmptyAndFileOnly();
    TestCloneCreateAndVisitor();
}
