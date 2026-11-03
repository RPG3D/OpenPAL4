#include "pal4/picture_preview_title.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PicturePreviewTitleRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> external;
    bool grow_external{};

    void Grow(pal4::PicturePreviewTitleState& state,
              std::uint32_t length) override {
        Check(length == 8u && state.Read(0u) == 0u &&
                  state.Read(4u) == 32u && state.Read(0x14u) == 0u,
              "picture title metadata initialized before grow");
        calls.emplace_back("grow");
        if (grow_external) {
            state.Write(4u, 64u);
            state.Write(0x94u, 0xb000u);
        }
    }
    void WriteCodepoint(std::uint32_t address,
                        std::uint32_t value) override {
        external[address] = value;
    }
    void Destroy(pal4::PicturePreviewTitleState&) override {
        calls.emplace_back("destroy");
    }
    int RegisterAtExit(std::uint32_t cleanup) override {
        Check(cleanup == 0x44bfa0u,
              "picture title cleanup address registered");
        calls.emplace_back("atexit");
        return 17;
    }
};

void CheckInline() {
    pal4::PicturePreviewTitleState state{};
    Runtime runtime;
    Check(pal4::PicturePreviewTitle::InitializeAndRegister(
              state, runtime) == 17 &&
              runtime.calls == std::vector<std::string>{"grow", "atexit"} &&
              state.Read(0u) == 8u && state.Read(4u) == 32u,
          "picture title initializes before atexit registration");
    constexpr char name[] = "biaoti20";
    for (std::uint32_t i = 0u; i < 8u; ++i)
        Check(state.Read(0x14u + i * 4u) ==
                  static_cast<std::uint8_t>(name[i]),
              "picture title bytes widened to CEGUI codepoints");
    Check(state.Read(0x34u) == 0u &&
              pal4::PicturePreviewTitle::kStateAddress == 0x8e43f0u &&
              pal4::PicturePreviewTitle::kInlineAddress == 0x8e4404u,
          "picture title NUL and global addresses");
    std::uint8_t guard = 0x80u;
    pal4::PicturePreviewTitle::Cleanup(state, guard, runtime);
    pal4::PicturePreviewTitle::Cleanup(state, guard, runtime);
    Check(guard == 0x81u && runtime.calls ==
              std::vector<std::string>{"grow", "atexit", "destroy"},
          "picture title destructor runs only once");
}

void CheckExternal() {
    pal4::PicturePreviewTitleState state{};
    Runtime runtime;
    runtime.grow_external = true;
    Check(pal4::PicturePreviewTitle::Initialize(state, runtime) == 0xb000u &&
              state.Read(0u) == 8u && runtime.external.size() == 9u,
          "grown CEGUI title uses external codepoint array");
    Check(runtime.external.at(0xb000u) == 'b' &&
              runtime.external.at(0xb01cu) == '0' &&
              runtime.external.at(0xb020u) == 0u,
          "grown picture title and wide NUL");
}

}  // namespace

int main() {
    CheckInline();
    CheckExternal();
    std::cout << "Picture preview title tests passed\n";
}
