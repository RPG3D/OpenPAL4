#include "pal4/ui_frame_resource_loader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct File {
    std::string name;
    bool special = false;
    bool directory = false;
};

struct Runtime final : pal4::IUiFrameResourceInitRuntime {
    std::vector<std::string> events;
    std::vector<std::uint32_t> stage_lines;
    std::vector<std::string> stage_formats;
    std::vector<std::uint32_t> file_log_lines;
    std::vector<std::string> file_log_names;
    std::vector<std::u32string> paths;
    std::vector<std::u32string> groups;
    std::array<std::vector<File>, 4> files;
    std::array<std::vector<std::string>, 4> package_files;
    std::array<bool, 4> begin_success{true, true, true, true};
    bool package_mode = false;
    bool fail_create = false;
    std::uint32_t read_address = 0;
    std::uint32_t provider = 0;
    int package_result = 0x1200;
    std::size_t active = 0;
    std::size_t next_scan = 0;
    std::size_t file_index = 0;

    static constexpr std::array<const char*, 4> kDirectories{
        "fonts", "imagesets", "schemes", "sequence_images"};
    static constexpr std::array<const char*, 4> kPatterns{
        "gamedata\\ui\\fonts\\*.font",
        "gamedata\\ui\\imagesets\\*.imageset",
        "gamedata\\ui\\schemes\\*.scheme",
        "gamedata\\ui\\sequence_images\\*.seqimage"};
    static constexpr std::array<const char*, 4> kStageFormats{
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager loading font...\n\n\n",
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager loading Imageset...\n\n\n",
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager loading scheme...\n\n\n",
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiFrameManager loading sequence image...\n\n\n"};

    void Create(const char* kind, std::u32string_view path,
                std::u32string_view group) {
        events.emplace_back(kind);
        paths.emplace_back(path);
        groups.emplace_back(group);
        if (fail_create) throw std::runtime_error("create failure");
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
    void Log(const char* source, std::uint32_t line,
             const char* format, const char* filename) override {
        Check(std::string_view(source).ends_with("uiFrameManager.cpp"),
              "file log source");
        Check(std::string_view(format).find("%s OK") !=
                  std::string_view::npos, "file log format");
        events.emplace_back("file log");
        file_log_lines.push_back(line);
        file_log_names.emplace_back(filename);
    }
    void SetSkinPath(std::string_view path) override {
        events.emplace_back("skin path");
        Check(path == "gamedata\\ui\\skin\\", "skin path bytes");
    }
    void SetSkinAutoLoad(bool enabled) override {
        events.emplace_back("skin flag");
        Check(!enabled, "skin autoload disabled");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read provider");
        read_address = address;
        return 0x9876u;
    }
    void SetWindowManagerResourceProvider(std::uint32_t value) override {
        events.emplace_back("set provider");
        provider = value;
    }
    bool UsePackageResources() override {
        events.emplace_back("mode");
        return package_mode;
    }
    void LogStage(const char* source, std::uint32_t line,
                  const char* format) override {
        events.emplace_back("stage");
        Check(std::string_view(source).ends_with("uiFrameManager.cpp"),
              "stage log source");
        stage_lines.push_back(line);
        stage_formats.emplace_back(format);
    }
    void InitializePkFileManager() override {
        events.emplace_back("pk init");
    }
    int EnumeratePackage(
        const char* directory,
        const std::function<int(const char*)>& callback) override {
        const auto scan = next_scan++;
        Check(scan < 4 && directory == std::string(kDirectories[scan]),
              "package directory order");
        events.emplace_back("package scan");
        for (const auto& name : package_files[scan]) {
            Check(callback(name.c_str()) == 1,
                  "PK callback continues enumeration");
        }
        return package_result + static_cast<int>(scan);
    }
    std::uint32_t ClearEnumerator() override {
        active = next_scan++;
        file_index = 0;
        events.emplace_back("clear");
        return static_cast<std::uint32_t>(active + 1u);
    }
    bool BeginEnumerator(std::uint32_t enumerator,
                         const char* pattern) override {
        Check(enumerator == active + 1u && active < 4 &&
                  pattern == std::string(kPatterns[active]),
              "local wildcard and enumerator handle");
        events.emplace_back("begin");
        return begin_success[active] && !files[active].empty();
    }
    const char* CurrentFilename(std::uint32_t enumerator) override {
        Check(enumerator == active + 1u, "current handle");
        events.emplace_back("current");
        return files[active][file_index].name.c_str();
    }
    bool IsSpecialDirectory(std::uint32_t enumerator) override {
        Check(enumerator == active + 1u, "special handle");
        events.emplace_back("special");
        return files[active][file_index].special;
    }
    bool IsDirectory(std::uint32_t enumerator) override {
        Check(enumerator == active + 1u, "directory handle");
        events.emplace_back("directory");
        return files[active][file_index].directory;
    }
    bool NextFile(std::uint32_t enumerator) override {
        Check(enumerator == active + 1u, "next handle");
        events.emplace_back("next");
        return ++file_index < files[active].size();
    }
    std::uint8_t EndEnumerator(std::uint32_t enumerator) override {
        Check(enumerator == active + 1u, "end handle");
        events.emplace_back("end");
        return static_cast<std::uint8_t>(0x80u + active);
    }
};

void CheckCommonPrefix(const Runtime& runtime) {
    Check(runtime.read_address == 0x51a4u &&
              runtime.provider == 0x9876u,
          "manager resource provider forwarded to WindowManager");
    Check(runtime.events.size() >= 5u &&
              std::vector<std::string>(runtime.events.begin(),
                                       runtime.events.begin() + 5) ==
                  std::vector<std::string>{"skin path", "skin flag",
                                           "read provider", "set provider",
                                           "mode"},
          "skin and provider precede resource enumeration");
    Check(runtime.stage_formats == std::vector<std::string>(
              Runtime::kStageFormats.begin(), Runtime::kStageFormats.end()),
          "stage log text and order");
}

void CheckPackagePath() {
    Runtime runtime;
    runtime.package_mode = true;
    runtime.package_files[0] = {"a.FONT", "ignored.txt"};
    runtime.package_files[1] = {"b.imageset"};
    runtime.package_files[2] = {"c.scheme"};
    runtime.package_files[3] = {"d.seqimage"};
    Check(pal4::UiFrameResourceInitializer::Initialize(0x5000u, runtime) ==
              0x03u, "package path returns low byte of final enumeration");
    CheckCommonPrefix(runtime);
    Check(runtime.stage_lines ==
              std::vector<std::uint32_t>{459u, 463u, 467u, 471u},
          "package source line numbers");
    Check(runtime.file_log_lines ==
              std::vector<std::uint32_t>{382u, 401u, 420u, 439u},
          "callback source line numbers");
    Check(runtime.file_log_names ==
              std::vector<std::string>{"a.FONT", "b.imageset",
                                       "c.scheme", "d.seqimage"},
          "callback logs matching names only");
    Check(runtime.paths == std::vector<std::u32string>{
              U"gamedata\\ui\\fonts\\a.FONT",
              U"gamedata\\ui\\imagesets\\b.imageset",
              U"gamedata\\ui\\schemes\\c.scheme",
              U"gamedata\\ui\\sequence_images\\d.seqimage"},
          "package callbacks reach the four CEGUI managers");
    for (const auto& group : runtime.groups)
        Check(group.empty(), "empty package CEGUI group");
    Check(runtime.next_scan == 4u &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         "pk init") == 4,
          "PK manager initialized for each directory");
}

void CheckLocalPath() {
    Runtime runtime;
    runtime.files[0] = {{"." , true, true}, {"folder", false, true},
                        {"a.font"}};
    runtime.files[1] = {{"b.imageset"}};
    runtime.files[2] = {{"c.scheme"}};
    runtime.files[3] = {{"d.seqimage"}};
    Check(pal4::UiFrameResourceInitializer::Initialize(0x5000u, runtime) ==
              0x83u, "local path returns final enumerator close byte");
    CheckCommonPrefix(runtime);
    Check(runtime.stage_lines ==
              std::vector<std::uint32_t>{480u, 500u, 521u, 542u},
          "local source line numbers");
    Check(runtime.file_log_names ==
              std::vector<std::string>{"a.font", "b.imageset",
                                       "c.scheme", "d.seqimage"},
          "special entries and directories skipped");
    Check(runtime.paths == std::vector<std::u32string>{
              U"gamedata\\ui\\fonts\\a.font",
              U"gamedata\\ui\\imagesets\\b.imageset",
              U"gamedata\\ui\\schemes\\c.scheme",
              U"gamedata\\ui\\sequence_images\\d.seqimage"},
          "local resource paths");
    Check(std::count(runtime.events.begin(), runtime.events.end(),
                     "end") == 4,
          "every local enumerator closed");
    Check(std::count(runtime.events.begin(), runtime.events.end(),
                     "directory") == 5,
          "special directory skips ordinary directory check");
}

void CheckEmptyAndExceptionalLocalEnumeration() {
    Runtime empty;
    empty.begin_success = {false, false, false, false};
    Check(pal4::UiFrameResourceInitializer::Initialize(0x5000u, empty) ==
              0x83u && empty.paths.empty() &&
              std::count(empty.events.begin(), empty.events.end(),
                         "end") == 4,
          "failed local starts still close every enumerator");

    Runtime failing;
    failing.files[0] = {{"a.font"}};
    failing.fail_create = true;
    bool threw = false;
    try {
        (void)pal4::UiFrameResourceInitializer::Initialize(
            0x5000u, failing);
    } catch (const std::runtime_error& error) {
        threw = std::string_view(error.what()) == "create failure";
    }
    Check(threw && failing.next_scan == 1u &&
              failing.file_log_names.empty() &&
              failing.events.back() == "end",
          "local creation failure closes enumerator and stops progression");
}

}  // namespace

int main() {
    try {
        CheckPackagePath();
        CheckLocalPath();
        CheckEmptyAndExceptionalLocalEnumeration();
        std::cout << "ui_frame_resource_initializer_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
