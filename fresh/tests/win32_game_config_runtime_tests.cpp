#include "pal4/binary_xml_config.h"
#include "pal4/win32_game_config_runtime.h"

#include <windows.h>
#include <mmsystem.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void Word(std::vector<std::uint8_t>& bytes, const std::uint32_t value) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes.push_back(static_cast<std::uint8_t>(value >> (index * 8u)));
    }
}

void Text(std::vector<std::uint8_t>& bytes,
          const std::string_view value) {
    Word(bytes, static_cast<std::uint32_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
}

struct Logger final : pal4::IGameConfigLogger {
    std::uint32_t logs = 0u;
    std::uint32_t InitializeLogger() override { return 0x123u; }
    void AppendLog(std::uint32_t logger, std::string_view,
                   std::string_view, std::uint32_t line) override {
        Check(logger == 0x123u && line == 0x87u,
              "native config logger forwarding");
        ++logs;
    }
};

void TestBinaryNodeShape() {
    std::vector<std::uint8_t> bytes;
    Word(bytes, 1u);
    Word(bytes, 1u);
    Text(bytes, "");
    Word(bytes, 2u);
    Word(bytes, 1u);
    Text(bytes, "width");
    Word(bytes, 1920u);
    Word(bytes, 3u);
    Text(bytes, "label");
    Text(bytes, "123");
    Word(bytes, 1u);
    Text(bytes, "child");
    Word(bytes, 1u);
    Word(bytes, 0u);
    Text(bytes, "flag");
    bytes.push_back(0xffu);
    Word(bytes, 0u);

    pal4::BinaryXmlConfig parsed;
    Check(parsed.Parse(bytes) && parsed.MajorVersion() == 1u &&
              parsed.MinorVersion() == 1u &&
              parsed.Root().FindAttribute("width")->ToInt(0u) == 1920u &&
              parsed.Root().FindAttribute("label")->ToInt(0u) == 123u &&
              parsed.Root().children.size() == 1u &&
              parsed.Root().children[0].name == "child" &&
              parsed.Root().children[0].FindAttribute("flag")->ToInt(0u) ==
                  0xffffffffu,
          "IDA binary node and attribute layout");
    bytes.pop_back();
    Check(!parsed.Parse(bytes), "truncated node is rejected");
    Check(parsed.MajorVersion() == 0u && parsed.MinorVersion() == 0u &&
              parsed.Root().attributes.empty(),
          "failed parse does not expose partial configuration");
    bytes[0] = 2u;
    Check(!parsed.Parse(bytes), "unsupported major version is rejected");

    pal4::BinaryXmlAttribute signed_text;
    signed_text.value = std::string{"\n+17 suffix"};
    Check(signed_text.ToInt(9u) == 17u,
          "integer text follows CRT decimal scanning");
    signed_text.value = std::string{"missing"};
    Check(signed_text.ToInt(9u) == 9u,
          "failed decimal scan keeps caller fallback");
}

void TestInstalledConfig(const std::filesystem::path& path) {
    pal4::BinaryXmlConfig parsed;
    Check(parsed.Load(path) && parsed.Root().name.empty() &&
              parsed.Root().attributes.size() == 5u &&
              parsed.Root().children.empty(),
          "installed config uses versioned binary XML root");
    const auto& root = parsed.Root();
    Check(root.FindAttribute("width")->ToInt(0u) == 1920u &&
              root.FindAttribute("height")->ToInt(0u) == 1080u &&
              root.FindAttribute("fullscreen")->ToInt(0u) == 1u &&
              root.FindAttribute("widescreen")->ToInt(0u) == 1u &&
              root.FindAttribute("sync")->ToInt(0u) == 1u,
          "five installed settings decode from their integer attributes");

    pal4::GameRenderState game{0x8e31e0u};
    pal4::GameConfigWords config{};
    pal4::GameConfigInitializer initializer{config, game};
    Logger logger;
    pal4::Win32GameConfigRuntime runtime{path.parent_path(), logger};
    const auto instance = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr)));
    Check(initializer.Initialize(instance, runtime) == 1u &&
              config[0] == 1920u && config[1] == 1080u &&
              game.Read(8u) == 1u && game.Read(0x0cu) == 1u &&
              game.Read(0x54cu) == 1u && logger.logs == 1u &&
              game.Read(0x18u) != 0u,
          "real config reaches reviewed game initializer through WinMM");
    (void)timeEndPeriod(game.Read(0x18u));
    Check(!parsed.Load(path.parent_path() / "missing-pal4-config.cfg") &&
              parsed.Root().attributes.empty(),
          "missing file clears previously loaded configuration");
}

}  // namespace

int main(int argc, char** argv) {
    TestBinaryNodeShape();
    if (argc > 1) TestInstalledConfig(argv[1]);
    return EXIT_SUCCESS;
}
