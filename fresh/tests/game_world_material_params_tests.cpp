#include "pal4/game_world_material_params.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldMaterialRuntime {
public:
    std::string ReadText(std::uint32_t address) override {
        Check(address == 1000u, "material reader uses persistent base path");
        calls.push_back("path");
        return "base\\scene\\";
    }
    std::uint32_t OpenDataStream(std::uint32_t kind, std::uint32_t mode,
                                 std::string_view path) override {
        Check(kind == 2u && mode == 1u && path == "base\\scene\\world.flr",
              "material stream opens world.flr in current world path");
        calls.push_back("open");
        return stream_result;
    }
    void ReadData(std::uint32_t stream, pal4::GameWorldObjectState& state,
                  std::uint32_t word_offset, std::uint32_t bytes) override {
        Check(stream == 0x7000u && bytes == 4u,
              "material fields read four bytes from open stream");
        calls.push_back("read");
        offsets.push_back(word_offset);
        state.words[word_offset] = 100u +
                                   static_cast<std::uint32_t>(offsets.size());
    }
    std::uint32_t CloseDataStream(std::uint32_t stream,
                                  std::uint32_t flags) override {
        Check(stream == 0x7000u && flags == 0u,
              "material stream closes with zero flags");
        calls.push_back("close");
        return 0x77u;
    }

    std::uint32_t stream_result = 0x7000u;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> offsets;
};

void ReadSixParameters() {
    pal4::GameWorldObjectState state;
    state.words.fill(0xdeadbeefu);
    state.words[1] = 1000u;
    Runtime runtime;
    Check(pal4::GameWorldMaterialParams::Load(state, runtime) == 0x77u &&
              runtime.offsets ==
                  std::vector<std::uint32_t>({10, 17, 22, 27, 32, 41}),
          "six material parameters load in IDA order and forward close result");
    for (std::uint32_t index = 0u; index < runtime.offsets.size(); ++index)
        Check(state.words[runtime.offsets[index]] == 101u + index,
              "stream writes each observed material word");
    Check(runtime.calls.front() == "path" && runtime.calls[1] == "open" &&
              runtime.calls.back() == "close" && runtime.calls.size() == 9u,
          "path construction, open, six reads and close preserve order");

    state.words[10] = 55u;
    Runtime missing;
    missing.stream_result = 0u;
    Check(pal4::GameWorldMaterialParams::Load(state, missing) == 0u &&
              state.words[10] == 55u && missing.offsets.empty() &&
              missing.calls == std::vector<std::string>({"path", "open"}),
          "missing world.flr preserves defaults and skips read/close");
}

}  // namespace

int main() {
    ReadSixParameters();
}
