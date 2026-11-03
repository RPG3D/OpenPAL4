#include "pal4/xml_serializer.h"
#include "pal4/compact_string_buffer.h"

#include <cstdlib>
#include <cstdio>
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

using File = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

File OpenFile() {
    File file(std::tmpfile(), &std::fclose);
    Check(file != nullptr, "temporary output file is available");
    return file;
}

std::string ReadAll(std::FILE* file) {
    std::fflush(file);
    std::fseek(file, 0, SEEK_END);
    const auto length = std::ftell(file);
    Check(length >= 0, "temporary file length is readable");
    std::rewind(file);
    std::string text(static_cast<std::size_t>(length), '\0');
    Check(std::fread(text.data(), 1u, text.size(), file) == text.size(),
          "temporary output can be read back");
    return text;
}

struct Runtime final : pal4::IXmlSerializationRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::string> virtual_text;
    std::map<std::uint32_t, std::uint32_t> next;
    std::map<std::uint32_t, bool> inline_nodes;
    std::vector<std::string> calls;
    std::uint32_t next_temporary{0x7000u};

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        calls.push_back("byte:" + std::to_string(address));
        return bytes.at(address);
    }
    std::string ReadText(const std::uint32_t address) override {
        calls.push_back("text:" + std::to_string(address));
        return texts.at(address);
    }
    std::uint32_t StoreTemporary(const std::string_view text) override {
        calls.push_back("store:" + std::string(text));
        if (text.empty()) return pal4::CompactStringBuffer::kEmptyBlock;
        const auto block = next_temporary;
        next_temporary += 0x100u;
        return block;
    }
    std::uint32_t FreeTemporary(const std::uint32_t block) override {
        calls.push_back("free:" + std::to_string(block));
        return 0xdeadu;
    }
    int WriteVirtual(const std::uint32_t node, std::FILE* stream,
                     const int depth, const std::uint32_t slot) override {
        calls.push_back("virtual:" + std::to_string(node) + ":" +
                        std::to_string(depth) + ":" +
                        std::to_string(slot));
        return std::fprintf(stream, "%s", virtual_text.at(node).c_str());
    }
    std::uint32_t NextSibling(const std::uint32_t node) override {
        calls.push_back("next:" + std::to_string(node));
        return next.at(node);
    }
    bool IsInline(const std::uint32_t node,
                  const std::uint32_t slot) override {
        calls.push_back("inline:" + std::to_string(node) + ":" +
                        std::to_string(slot));
        return inline_nodes.at(node);
    }
};

Runtime MakeNode() {
    Runtime runtime;
    runtime.words[0x1020u] = 0x5000u;
    runtime.texts[0x5008u] = "root";
    runtime.words[0x104cu] = 0x102cu;
    runtime.words[0x1018u] = 0u;
    return runtime;
}

void TestWrapperAndEmptyNode() {
    auto runtime = MakeNode();
    runtime.words[0x3014u] = 0x5000u;
    runtime.words[0x3018u] = 0x5100u;
    runtime.texts[0x5008u] = "name";
    runtime.texts[0x5108u] = "value";
    auto file = OpenFile();
    Check(pal4::XmlSerializer::WriteAttribute(
              0x3000u, file.get(), 2, runtime) == 0xdeadu &&
              ReadAll(file.get()) == "name=\"value\"" &&
              runtime.calls == std::vector<std::string>{
                  "read:12308", "read:12312", "text:20488", "text:20744",
                  "store:name", "store:value", "free:28928", "free:28672"},
          "file wrapper keeps attribute pointer and releases value before name");

    file = OpenFile();
    runtime.texts[0x5008u] = "root";
    runtime.calls.clear();
    Check(pal4::XmlSerializer::WriteNode(
              0x1000u, file.get(), 0, runtime) == 3 &&
              ReadAll(file.get()) == "<root />" &&
              runtime.calls == std::vector<std::string>{
                  "read:4128", "text:20488", "read:4172", "read:4120"},
          "empty XML node emits self-closing tag and returns last print count");
}

void TestEscapedAttribute() {
    Runtime runtime;
    runtime.words[0x3014u] = 0x5000u;
    runtime.words[0x3018u] = 0x5100u;
    runtime.texts[0x5008u] = "a&b";
    runtime.texts[0x5108u] = "x\"y'";
    auto file = OpenFile();
    std::string appended{"prefix:"};
    Check(pal4::XmlSerializer::WriteAttributeRecord(
              0x3000u, file.get(), 4, &appended, runtime) == 0xdeadu &&
              ReadAll(file.get()) == "a&amp;b='x&quot;y&apos;'" &&
              appended == "prefix:a&amp;b='x&quot;y&apos;'",
          "raw double quote selects single delimiter after XML escaping");
    runtime.texts[0x5008u].clear();
    runtime.texts[0x5108u].clear();
    runtime.calls.clear();
    appended.clear();
    Check(pal4::XmlSerializer::WriteAttributeRecord(
              0x3000u, nullptr, 0, &appended, runtime) ==
              pal4::CompactStringBuffer::kEmptyBlock &&
              appended == "=\"\"" &&
              runtime.calls.size() == 6u &&
              runtime.calls.back() == "store:",
          "empty escaped buffers use sentinel and need no release");
}

void TestAttributeAndInlineChild() {
    auto runtime = MakeNode();
    runtime.words[0x104cu] = 0x3000u;
    runtime.next[0x3000u] = 0u;
    runtime.virtual_text[0x3000u] = "id=\"7\"";
    runtime.words[0x1018u] = 0x4000u;
    runtime.words[0x101cu] = 0x4000u;
    runtime.inline_nodes[0x4000u] = true;
    runtime.virtual_text[0x4000u] = "hello";
    auto file = OpenFile();
    Check(pal4::XmlSerializer::WriteNode(
              0x1000u, file.get(), 1, runtime) == 7 &&
              ReadAll(file.get()) == "    <root id=\"7\">hello</root>" &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "inline:16384:44" &&
              runtime.calls.back() == "virtual:16384:2:4",
          "one inline child stays on the same line after attributes");
}

void TestMixedChildren() {
    auto runtime = MakeNode();
    runtime.words[0x1018u] = 0x4000u;
    runtime.words[0x101cu] = 0x4100u;
    runtime.words[0x4028u] = 0x4100u;
    runtime.words[0x4128u] = 0u;
    runtime.inline_nodes[0x4000u] = true;
    runtime.inline_nodes[0x4100u] = false;
    runtime.virtual_text[0x4000u] = "text";
    runtime.virtual_text[0x4100u] = "<nested />";
    auto file = OpenFile();
    Check(pal4::XmlSerializer::WriteNode(
              0x1000u, file.get(), 0, runtime) == 7 &&
              ReadAll(file.get()) ==
                  "<root>text\n<nested />\n</root>",
          "mixed children receive newline only before non-inline entries");
    pal4::XmlSerializationRuntimeStub stub;
    Check(stub.ReadWord(0u) == 0u && stub.ReadText(0u).empty() &&
              !stub.IsInline(0u, 44u),
          "native XML serialization boundary has a linkable stub");
}

void TestWriteChildren() {
    auto runtime = MakeNode();
    auto file = OpenFile();
    Check(pal4::XmlSerializer::WriteChildren(
              0x1000u, file.get(), 2, runtime) == 0 &&
              ReadAll(file.get()).empty(),
          "empty child list produces no file output");
    runtime.words[0x1018u] = 0x4000u;
    runtime.words[0x4028u] = 0x4100u;
    runtime.words[0x4128u] = 0u;
    runtime.virtual_text[0x4000u] = "one";
    runtime.virtual_text[0x4100u] = "two";
    runtime.calls.clear();
    file = OpenFile();
    Check(pal4::XmlSerializer::WriteChildren(
              0x1000u, file.get(), 2, runtime) == 1 &&
              ReadAll(file.get()) == "one\ntwo\n" &&
              runtime.calls == std::vector<std::string>{
                  "read:4120", "virtual:16384:2:4", "read:16424",
                  "virtual:16640:2:4", "read:16680"},
          "each child is written through slot four followed by one newline");
}

void TestWriteCData() {
    Runtime runtime;
    runtime.words[0x1020u] = 0x5000u;
    runtime.texts[0x5008u] = "a<&";
    runtime.bytes[0x102cu] = 1u;
    auto file = OpenFile();
    Check(pal4::XmlSerializer::WriteCData(
              0x1000u, file.get(), 2, runtime) == 16u &&
              ReadAll(file.get()) == "\n        <![CDATA[a<&]]>\n" &&
              runtime.calls == std::vector<std::string>{
                  "byte:4140", "read:4128", "text:20488"},
          "CDATA flag writes newline, indentation and raw text");
    runtime.bytes[0x102cu] = 0u;
    runtime.calls.clear();
    file = OpenFile();
    Check(pal4::XmlSerializer::WriteCData(
              0x1000u, file.get(), 5, runtime) == 0xdeadu &&
              ReadAll(file.get()) == "a&lt;&amp;" &&
              runtime.calls == std::vector<std::string>{
                  "byte:4140", "read:4128", "text:20488",
                  "store:a&lt;&amp;", "free:28672"},
          "plain text is escaped without indentation and frees temporary");
    runtime.texts[0x5008u].clear();
    runtime.calls.clear();
    file = OpenFile();
    Check(pal4::XmlSerializer::WriteCData(
              0x1000u, file.get(), 0, runtime) ==
              pal4::CompactStringBuffer::kEmptyBlock &&
              ReadAll(file.get()).empty() &&
              runtime.calls.back() == "store:",
          "empty plain text keeps shared temporary block");
}

void TestWriteOpenTag() {
    auto runtime = MakeNode();
    auto file = OpenFile();
    Check(pal4::XmlSerializer::WriteOpenTag(
              0x1000u, file.get(), 2, runtime) == 6 &&
              ReadAll(file.get()) == "        <root>" &&
              runtime.calls == std::vector<std::string>{
                  "read:4128", "text:20488"},
          "open tag writer indents and returns the final print count");
    file = OpenFile();
    runtime.calls.clear();
    Check(pal4::XmlSerializer::WriteOpenTag(
              0x1000u, file.get(), -1, runtime) == 6 &&
              ReadAll(file.get()) == "<root>",
          "nonpositive depth emits no indentation");
}

}  // namespace

int main() {
    TestWrapperAndEmptyNode();
    TestEscapedAttribute();
    TestAttributeAndInlineChild();
    TestMixedChildren();
    TestWriteChildren();
    TestWriteCData();
    TestWriteOpenTag();
}
