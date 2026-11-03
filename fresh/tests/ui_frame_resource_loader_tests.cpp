#include "pal4/ui_frame_resource_loader.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameResourceRuntime {
    std::vector<std::string> events;
    std::u32string path;
    std::u32string group;
    std::string source;
    std::string format;
    std::string filename;
    std::uint32_t line = 0;
    bool throw_on_create = false;
    bool throw_on_log = false;

    void Create(const char* kind, std::u32string_view resource_path,
                std::u32string_view resource_group) {
        events.emplace_back(kind);
        path = resource_path;
        group = resource_group;
        if (throw_on_create) throw std::runtime_error("create failure");
    }
    void CreateFont(std::u32string_view path,
                    std::u32string_view group) override {
        Create("font", path, group);
    }
    void CreateImageset(std::u32string_view path,
                        std::u32string_view group) override {
        Create("imageset", path, group);
    }
    void LoadScheme(std::u32string_view path,
                    std::u32string_view group) override {
        Create("scheme", path, group);
    }
    void CreateSequenceImage(std::u32string_view path,
                             std::u32string_view group) override {
        Create("sequence", path, group);
    }
    void Log(const char* log_source, std::uint32_t log_line,
             const char* log_format, const char* log_filename) override {
        events.emplace_back("log");
        source = log_source;
        line = log_line;
        format = log_format;
        filename = log_filename;
        if (throw_on_log) throw std::runtime_error("log failure");
    }
};

using LoaderMethod = int (pal4::UiFrameResourceLoader::*)(const char*);

struct Case {
    LoaderMethod method;
    const char* filename;
    const char* extension;
    const char* kind;
    std::u32string_view path;
    std::uint32_t line;
    const char* format;
};

constexpr Case kCases[] = {
    {&pal4::UiFrameResourceLoader::LoadFontFile, "a.font", ".font",
     "font", U"gamedata\\ui\\fonts\\a.font", 382u,
     "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager load font %s OK\n\n\n"},
    {&pal4::UiFrameResourceLoader::LoadImagesetFile,
     "a.imageset", ".imageset", "imageset",
     U"gamedata\\ui\\imagesets\\a.imageset", 401u,
     "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager create Imageset %s OK\n\n\n"},
    {&pal4::UiFrameResourceLoader::LoadSchemeFile,
     "a.scheme", ".scheme", "scheme",
     U"gamedata\\ui\\schemes\\a.scheme", 420u,
     "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager load scheme %s OK\n\n\n"},
    {&pal4::UiFrameResourceLoader::LoadSequenceImageFile,
     "a.seqimage", ".seqimage", "sequence",
     U"gamedata\\ui\\sequence_images\\a.seqimage", 439u,
     "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager load sequence image %s OK\n\n\n"},
};

void CheckAllKinds() {
    for (const auto& item : kCases) {
        Runtime runtime;
        pal4::UiFrameResourceLoader loader(runtime);
        Check((loader.*item.method)(item.filename) == 1,
              "matching callback returns enumeration continuation");
        Check(runtime.events == std::vector<std::string>{item.kind, "log"},
              "CEGUI creation precedes success log");
        Check(runtime.path == item.path && runtime.group.empty(),
              "exact CEGUI path and empty resource group");
        Check(runtime.line == item.line && runtime.format == item.format &&
                  runtime.filename == item.filename,
              "exact log arguments");
        Check(runtime.source ==
                  "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\UiFrame\\uiFrameManager.cpp",
              "exact source file");

        runtime.events.clear();
        Check((loader.*item.method)(item.extension) == 1 &&
                  runtime.events.empty(),
              "extension alone fails strict length gate");
        Check((loader.*item.method)("other.txt") == 1 &&
                  runtime.events.empty(),
              "wrong suffix has no side effects");
        Check((loader.*item.method)("") == 1 && runtime.events.empty(),
              "empty name has no side effects");

        std::string uppercase = "subdir\\a";
        for (const unsigned char ch : std::string_view(item.extension)) {
            uppercase += ch >= 'a' && ch <= 'z'
                ? static_cast<char>(ch - ('a' - 'A')) : static_cast<char>(ch);
        }
        Check((loader.*item.method)(uppercase.c_str()) == 1 &&
                  runtime.events == std::vector<std::string>{item.kind, "log"},
              "uppercase suffix accepted and subdirectory preserved");
        Check(runtime.path.size() >= uppercase.size() &&
                  runtime.path.substr(runtime.path.size() - uppercase.size()) ==
                      std::u32string(uppercase.begin(), uppercase.end()),
              "case and path separators preserved in CEGUI argument");
    }
}

void CheckByteWideningAndLongName() {
    Runtime runtime;
    pal4::UiFrameResourceLoader loader(runtime);
    const char high_byte_name[] = {'a', static_cast<char>(0xe9), '.',
                                   'F', 'o', 'N', 't', '\0'};
    Check(loader.LoadFontFile(high_byte_name) == 1,
          "high-byte filename accepted");
    Check(runtime.path == std::u32string(U"gamedata\\ui\\fonts\\a") +
                              char32_t{0xe9} + U".FoNt",
          "CEGUI receives unsigned byte values without UTF-8 decoding");

    const std::string long_name(100, 'x');
    const std::string filename = long_name + ".scheme";
    runtime.events.clear();
    Check(loader.LoadSchemeFile(filename.c_str()) == 1 &&
              runtime.path.size() == std::string("gamedata\\ui\\schemes\\").size() +
                                         filename.size() &&
              runtime.events == std::vector<std::string>{"scheme", "log"},
          "long path crosses CEGUI inline storage capacity");
}

void CheckExceptions() {
    for (const auto& item : kCases) {
        Runtime runtime;
        pal4::UiFrameResourceLoader loader(runtime);
        runtime.throw_on_create = true;
        bool threw = false;
        try {
            (void)(loader.*item.method)(item.filename);
        } catch (const std::runtime_error& error) {
            threw = std::string_view(error.what()) == "create failure";
        }
        Check(threw && runtime.events == std::vector<std::string>{item.kind},
              "CEGUI exception propagates without success log");

        runtime.events.clear();
        runtime.throw_on_create = false;
        runtime.throw_on_log = true;
        threw = false;
        try {
            (void)(loader.*item.method)(item.filename);
        } catch (const std::runtime_error& error) {
            threw = std::string_view(error.what()) == "log failure";
        }
        Check(threw &&
                  runtime.events == std::vector<std::string>{item.kind, "log"},
              "logger exception propagates after CEGUI call");
    }
}

}  // namespace

int main() {
    try {
        CheckAllKinds();
        CheckByteWideningAndLongName();
        CheckExceptions();
        std::cout << "ui_frame_resource_loader_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
