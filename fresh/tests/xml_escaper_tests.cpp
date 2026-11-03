#include "pal4/xml_escaper.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestSpecialCharacters() {
    std::string output{"prefix:"};
    auto& result = pal4::XmlEscaper::Append(output, "a&<>\"'z");
    Check(&result == &output &&
              output == "prefix:a&amp;&lt;&gt;&quot;&apos;z",
          "escape appends five XML entities to existing output");
    pal4::XmlEscaper::Append(output, "");
    Check(output == "prefix:a&amp;&lt;&gt;&quot;&apos;z",
          "empty input leaves the destination unchanged");
}

void TestControlsAndHighBytes() {
    const char bytes[]{'\0', '\x01', '\x1f', '\x20', '\x7f',
                       static_cast<char>(0x80), static_cast<char>(0xff)};
    std::string output;
    pal4::XmlEscaper::Append(output,
                             std::string_view(bytes, sizeof(bytes)));
    const std::string expected = std::string("&#x00;&#x01;&#x1F; ") +
                                 static_cast<char>(0x7f) +
                                 static_cast<char>(0x80) +
                                 static_cast<char>(0xff);
    Check(output == expected,
          "control bytes use uppercase two-digit hexadecimal entities");
}

void TestNumericHexPrefix() {
    std::string output;
    pal4::XmlEscaper::Append(output, "&#x26; &x &#X26; &#xQ; &#x1<");
    Check(output == "&#x26; &amp;x &amp;#X26; &#xQ; &#x1&lt;",
          "lowercase numeric-hex prefix is preserved through final byte");
    output.clear();
    pal4::XmlEscaper::Append(output, "&#x");
    Check(output == "&#x", "short numeric-hex prefix remains intact");
    output.clear();
    pal4::XmlEscaper::Append(output, "&#");
    Check(output == "&amp;#", "incomplete prefix escapes ampersand");
    output = "&";
    pal4::XmlEscaper::Append(output, output);
    Check(output == "&&amp;", "self-append keeps a stable source snapshot");
}

}  // namespace

int main() {
    TestSpecialCharacters();
    TestControlsAndHighBytes();
    TestNumericHexPrefix();
}
