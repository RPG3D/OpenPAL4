#include "pal4/movie_preview_title.h"

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

struct Runtime final : pal4::MoviePreviewTitleRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> external;
    bool external_buffer{};
    int atexit_result{-7};

    void Grow(pal4::MoviePreviewTitleState& state,
              std::uint32_t length) override {
        Check(length == 8u && state.Read(0u) == 0u &&
                  state.Read(4u) == 32u && state.Read(0x14u) == 0u,
              "CEGUI string metadata initialized before grow");
        calls.emplace_back("grow");
        if (external_buffer) {
            state.Write(4u, 64u);
            state.Write(0x94u, 0xb000u);
        }
    }
    void WriteCodepoint(std::uint32_t address,
                        std::uint32_t codepoint) override {
        external[address] = codepoint;
        calls.emplace_back("external-write");
    }
    void Destroy(pal4::MoviePreviewTitleState&) override {
        calls.emplace_back("destroy");
    }
    int RegisterAtExit(std::uint32_t address) override {
        Check(address == 0x44b100u, "title cleanup registered at atexit");
        calls.emplace_back("atexit");
        return atexit_result;
    }
};

void CheckInlineAndAtexit() {
    pal4::MoviePreviewTitleState state{};
    Runtime runtime;
    Check(pal4::MoviePreviewTitle::InitializeAndRegister(state, runtime) ==
                  -7 && state.Read(0u) == 8u &&
              state.Read(4u) == 32u &&
              runtime.calls == std::vector<std::string>{"grow", "atexit"},
          "short title is initialized before cleanup registration");
    constexpr char title[] = "biaoti30";
    for (std::uint32_t i = 0u; i < 8u; ++i)
        Check(state.Read(0x14u + i * 4u) ==
                  static_cast<std::uint8_t>(title[i]),
              "title bytes widened to CEGUI codepoints");
    Check(state.Read(0x34u) == 0u, "title has wide NUL terminator");
    std::uint8_t guard = 0x80u;
    pal4::MoviePreviewTitle::Cleanup(state, guard, runtime);
    pal4::MoviePreviewTitle::Cleanup(state, guard, runtime);
    Check(guard == 0x81u && runtime.calls ==
              std::vector<std::string>{"grow", "atexit", "destroy"},
          "cleanup guard preserves other bits and destroys once");
}

void CheckExternalBuffer() {
    pal4::MoviePreviewTitleState state{};
    Runtime runtime;
    runtime.external_buffer = true;
    Check(pal4::MoviePreviewTitle::Initialize(state, runtime) == 0xb000u &&
              state.Read(0u) == 8u && runtime.external.size() == 9u,
          "grown title writes nine codepoints to external buffer");
    constexpr char title[] = "biaoti30";
    for (std::uint32_t i = 0u; i < 8u; ++i)
        Check(runtime.external.at(0xb000u + i * 4u) ==
                  static_cast<std::uint8_t>(title[i]),
              "external title codepoint values");
    Check(runtime.external.at(0xb020u) == 0u,
          "external title NUL terminator");
    std::uint8_t guard = 1u;
    pal4::MoviePreviewTitle::Cleanup(state, guard, runtime);
    Check(runtime.calls.size() == 10u,
          "already-cleaned title skips destructor");
}

}  // namespace

int main() {
    CheckInlineAndAtexit();
    CheckExternalBuffer();
    std::cout << "Movie preview title tests passed\n";
}
