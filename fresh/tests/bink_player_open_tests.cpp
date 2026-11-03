#include "pal4/bink_player.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IBinkPlayerOpenRuntime {
public:
    void EnsureSingletonManager() override {}
    std::uint32_t RegisterSingleton(
        std::string_view, std::uint32_t,
        std::function<std::byte*()>,
        std::function<std::uint32_t(std::byte*)>) override { return 0u; }
    std::byte* Allocate(std::size_t) override { return nullptr; }
    std::uint32_t Free(std::byte*) override { return 0u; }
    void InitializePackage(std::byte*) override {}
    std::uint32_t ClosePackage(std::byte* package) override {
        events.push_back("close-package-" + Slot(package));
        return 0x55u;
    }
    std::uint32_t CleanupPackage(std::byte*) override { return 0u; }

    float VideoMasterVolume() override {
        events.emplace_back("master-volume");
        return 0.5f;
    }
    bool UseVideoPackages() override {
        events.emplace_back("use-packages");
        return packaged;
    }
    std::string ScriptVideoName(std::uint32_t source) override {
        events.emplace_back("script-name");
        Check(source == 0x123u, "script lookup receives caller source");
        return "intro.bik";
    }
    void ResetPackageGlobalPointer(
        std::byte* package, std::uint32_t value) override {
        events.push_back("reset-" + Slot(package));
        Check(value == 1u, "package callback mode set to one");
    }
    bool FileExists(std::string_view path) override {
        events.emplace_back("exists");
        last_path = path;
        if (path.starts_with("gamedata\\"))
            return relative_exists;
        return prompts >= prompts_needed;
    }
    std::string InstallRoot() override {
        events.emplace_back("root");
        return "D:\\";
    }
    void PromptForDisk(char drive) override {
        events.emplace_back("prompt");
        Check(drive == 'D', "prompt names configured drive");
        ++prompts;
    }
    bool OpenPackage(std::byte* package,
                     std::string_view path) override {
        const auto slot = Slot(package);
        events.push_back("open-package-" + slot);
        last_path = path;
        return slot == "first" ? first_package_ok : second_package_ok;
    }
    std::uint32_t OpenPackageFile(
        std::byte* package, std::string_view path) override {
        const auto slot = Slot(package);
        events.push_back("open-file-" + slot);
        Check(path == "intro.bik", "package opens script video name");
        return slot == "first" ? first_file : second_file;
    }
    void SeekPackageFile(std::byte* package, std::uint32_t file,
                         std::uint32_t offset,
                         std::uint32_t origin) override {
        const auto slot = Slot(package);
        events.push_back("seek-" + slot);
        Check(file == (slot == "first" ? 0xa1u : 0xb1u) &&
                  offset == 0u && origin == 0u,
              "package video seek starts at origin");
    }
    std::uint32_t PackageBinkSource(std::byte* package) override {
        const auto slot = Slot(package);
        events.push_back("source-" + slot);
        return slot == "first" ? 0xa000u : 0xb000u;
    }
    void ClosePackageFile(std::byte* package,
                          std::uint32_t file) override {
        const auto slot = Slot(package);
        events.push_back("close-file-" + slot);
        Check(file == (slot == "first" ? 0xa1u : 0xb1u),
              "closed file matches opened package handle");
    }
    std::uint32_t ViewportWidth() override { return 800u; }
    std::uint32_t ViewportHeight() override { return 600u; }
    std::uint32_t CreateTexture(std::uint32_t) override {
        events.emplace_back("parameters");
        return 0x400u;
    }

    std::uint32_t OpenBink(std::uint32_t source,
                            bool use_io_table) override {
        events.emplace_back("bink-open");
        opened_source = source;
        opened_with_table = use_io_table;
        return bink_handle;
    }
    std::uint32_t CloseBink(std::uint32_t) override {
        events.emplace_back("bink-close");
        return 0u;
    }
    std::uint32_t SetBinkVolume(
        std::uint32_t, std::uint32_t track,
        std::uint32_t scaled) override {
        events.emplace_back("volume");
        Check(track == 0u && scaled == 32767u,
              "video master volume reaches Bink track zero");
        return 0u;
    }
    std::uint32_t CreateRaster(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    std::uint32_t VideoWidth(std::uint32_t) override { return 0u; }
    std::uint32_t VideoHeight(std::uint32_t) override { return 0u; }
    std::uint8_t RasterFlags(std::uint32_t) override { return 0u; }
    void LogFailure(std::uint32_t) override {}
    bool IsPaused(std::uint32_t) override { return false; }
    std::uint32_t PauseBink(std::uint32_t, bool) override { return 0u; }
    std::uint32_t FrameNumber(std::uint32_t) override { return 0u; }
    std::uint32_t FrameCount(std::uint32_t) override { return 0u; }
    std::uint32_t WaitBink(std::uint32_t) override { return 0u; }
    void DoFrame(std::uint32_t) override {}
    std::uint32_t RasterWidth(std::uint32_t) override { return 0u; }
    std::uint32_t RasterStride(std::uint32_t) override { return 0u; }
    std::uint32_t LockRaster(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    void CopyToBuffer(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::int32_t, std::int32_t,
        std::uint32_t) override {}
    void UnlockRaster(std::uint32_t) override {}
    void NextFrame(std::uint32_t) override {}

    std::string Slot(std::byte* package) const {
        if (package == object + pal4::BinkPlayer::kFirstPackageOffset)
            return "first";
        Check(package == object + pal4::BinkPlayer::kSecondPackageOffset,
              "package pointer uses known slot");
        return "second";
    }

    std::byte* object = nullptr;
    std::vector<std::string> events;
    std::string last_path;
    std::uint32_t opened_source = 0u;
    std::uint32_t bink_handle = 0x700u;
    std::uint32_t first_file = 0xa1u;
    std::uint32_t second_file = 0xb1u;
    bool packaged = false;
    bool centered = false;
    bool relative_exists = true;
    bool first_package_ok = true;
    bool second_package_ok = true;
    bool opened_with_table = false;
    unsigned prompts = 0u;
    unsigned prompts_needed = 0u;
};

bool Contains(const std::vector<std::string>& events,
              const std::string& event) {
    return std::find(events.begin(), events.end(), event) != events.end();
}

void DirectAndFirstPackage() {
    std::vector<std::byte> object(pal4::BinkPlayer::kSize);
    Runtime runtime;
    runtime.object = object.data();
    runtime.centered = true;
    Check(pal4::BinkPlayer::OpenVideoFile(
              object.data(), 0x123u, true, runtime) &&
              runtime.opened_source == 0x123u &&
              !runtime.opened_with_table &&
              runtime.events == std::vector<std::string>{
                  "master-volume", "use-packages", "bink-open",
                  "volume", "parameters"},
          "direct mode opens source without I/O table then sets parameters");
    runtime.events.clear();
    runtime.bink_handle = 0u;
    Check(!pal4::BinkPlayer::OpenVideoFile(
              object.data(), 0x123u, true, runtime) &&
              runtime.events == std::vector<std::string>{
                  "master-volume", "use-packages", "bink-close",
                  "bink-open"},
          "failed direct open closes old video and omits parameters");
    runtime.events.clear();
    runtime.bink_handle = 0x700u;
    runtime.packaged = true;
    object[pal4::BinkPlayer::kFirstPackageOpenFlag] = std::byte{1};
    object[pal4::BinkPlayer::kSecondPackageOpenFlag] = std::byte{1};
    Check(pal4::BinkPlayer::OpenVideoFile(
              object.data(), 0x123u, true, runtime) &&
              runtime.opened_source == 0xa000u &&
              runtime.opened_with_table &&
              runtime.last_path == "gamedata\\VideoA.cpk" &&
              Contains(runtime.events, "close-package-first") &&
              Contains(runtime.events, "close-package-second") &&
              Contains(runtime.events, "reset-first") &&
              Contains(runtime.events, "reset-second") &&
              Contains(runtime.events, "seek-first") &&
              Contains(runtime.events, "close-file-first") &&
              runtime.events.back() == "parameters",
          "first package closes old slots and opens video through callback");
}

void FallbackAndFailures() {
    std::vector<std::byte> object(pal4::BinkPlayer::kSize);
    Runtime runtime;
    runtime.object = object.data();
    runtime.packaged = true;
    runtime.first_file = 0u;
    runtime.relative_exists = false;
    runtime.prompts_needed = 2u;
    Check(pal4::BinkPlayer::OpenVideoFile(
              object.data(), 0x123u, false, runtime) &&
              runtime.prompts == 2u &&
              runtime.last_path == "D:\\PAL4\\gamedata\\VideoB.cpk" &&
              runtime.opened_source == 0xb000u &&
              Contains(runtime.events, "close-package-first") &&
              Contains(runtime.events, "seek-second") &&
              Contains(runtime.events, "close-file-second") &&
              runtime.events.back() == "parameters",
          "missing first file falls back to second package and disk path");
    runtime.events.clear();
    runtime.second_file = 0u;
    Check(pal4::BinkPlayer::OpenVideoFile(
              object.data(), 0x123u, false, runtime) &&
              !Contains(runtime.events, "bink-open") &&
              runtime.events.back() == "parameters",
          "native second-package missing-file branch returns success");
    runtime.events.clear();
    runtime.first_package_ok = false;
    Check(!pal4::BinkPlayer::OpenVideoFile(
              object.data(), 0x123u, false, runtime) &&
              !Contains(runtime.events, "open-file-first") &&
              !Contains(runtime.events, "parameters"),
          "first archive open failure ends before file lookup");
}

}  // namespace

int main() {
    DirectAndFirstPackage();
    FallbackAndFailures();
    std::cout << "Bink player open tests passed\n";
}
