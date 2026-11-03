#include "pal4/minimap_manager.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class AccessRuntime final : public pal4::IMinimapManagerAccessRuntime {
public:
    std::vector<std::string> events;
    std::uint32_t result = 0x7000u;

    std::uint32_t GetSingletonManager() override {
        events.emplace_back("manager");
        return 0x1234u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t destroy) override {
        Check(manager == 0x1234u && name == "palColorMiniMapManager" &&
                  id == 0x40bu && factory == 0x40dc70u &&
                  destroy == 0x40dcd0u,
              "singleton registration arguments");
        events.emplace_back("register");
        return result;
    }
};

class ObjectRuntime final : public pal4::IMinimapManagerObjectRuntime {
public:
    pal4::MinimapManagerObjectState state;
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocated = 0x9000u;
    std::uint32_t texture_allocated = 0x7000u;
    std::uint32_t texture_result = 0x7000u;
    std::uint32_t pixels = 0x5000u;
    bool throw_on_construct = false;
    bool throw_on_texture = false;
    bool shrink_on_grid_cleanup = false;
    bool shrink_on_read = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        if (size == 0x58u) {
            events.emplace_back("allocate");
            return allocated;
        }
        Check(size == 0x2cu, "texture allocation is 44 bytes");
        events.emplace_back("allocate-texture");
        return texture_allocated;
    }
    pal4::MinimapManagerObjectState& Access(std::uint32_t address) override {
        Check(address == 0x9000u, "object address");
        events.emplace_back("access");
        return state;
    }
    void BaseConstruct(std::uint32_t address) override {
        Check(address == 0x9000u, "base constructor address");
        events.emplace_back("base-construct");
        if (throw_on_construct) throw std::runtime_error("base");
    }
    void CleanupCollection(std::uint32_t address) override {
        Check(address == 0x9000u &&
                  state.ReadDword(0x18u) == 0u &&
                  state.ReadDword(0x1cu) == 0u &&
                  state.ReadDword(0x20u) == 0u,
              "collection cleanup follows field clearing");
        events.emplace_back("cleanup-collection");
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }
    void ResetTexture(std::uint32_t address) override {
        events.emplace_back("reset-texture:" + std::to_string(address));
    }
    std::uint32_t GridPixelBuffer(std::uint32_t grid) override {
        Check(grid == 0x5555u, "texture grid address");
        events.emplace_back("grid-pixels");
        return pixels;
    }
    std::uint32_t InitializeTexture(
        std::uint32_t address,
        const std::array<std::uint32_t, 4>& region,
        std::uint32_t grid, bool mode) override {
        Check(address == 0x7000u &&
                  region == std::array<std::uint32_t, 4>{0u, 0u, 12u, 34u} &&
                  grid == 0x5555u && mode == expected_mode,
              "texture constructor arguments");
        events.emplace_back("initialize-texture");
        if (throw_on_texture) throw std::runtime_error("texture");
        return texture_result;
    }
    bool expected_mode = true;
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        if (shrink_on_read && address == 0x4000u) {
            state.WriteDword(0x1cu, 0x4004u);
        }
        return words[address];
    }
    void CleanupGrid(std::uint32_t address) override {
        events.emplace_back("cleanup-grid:" + std::to_string(address));
        if (shrink_on_grid_cleanup) {
            state.WriteDword(0x1cu, state.ReadDword(0x18u) + 4u);
        }
    }
    std::uint32_t CleanupObjects(std::uint32_t address) override {
        Check(address == 0x9000u && state.ReadDword(0x24u) == 0u &&
                  state.ReadDword(0x1cu) == state.ReadDword(0x18u),
              "object cleanup follows list reset");
        events.emplace_back("cleanup-objects");
        return 0x7777u;
    }
    void LogInvalidGridIndex(std::uint32_t index) override {
        events.emplace_back("invalid-index:" + std::to_string(index));
    }
    void InitializeDialogSystem(std::uint32_t manager) override {
        Check(manager == 0x9000u, "dialog initialization manager");
        events.emplace_back("init-dialog");
    }
};

class DrawRuntime final : public pal4::IMinimapManagerDrawRuntime {
public:
    pal4::MinimapManagerObjectState state;
    std::vector<std::string> events;
    std::uint32_t texture = 0x9000u;
    bool expected_alpha_mode = true;

    pal4::MinimapManagerObjectState& Access(std::uint32_t address) override {
        Check(address == 0x8000u, "draw manager address");
        events.emplace_back("access");
        return state;
    }
    std::uint32_t TextureField(std::uint32_t address,
                               std::uint32_t offset) override {
        Check(address == 0x7000u &&
                  (offset == 0x28u || offset == 4u || offset == 8u),
              "texture handle field");
        events.emplace_back("texture-field:" + std::to_string(offset));
        if (offset == 4u) return 10u;
        if (offset == 8u) return 20u;
        return texture;
    }
    void CommitTextureRaster(std::uint32_t address) override {
        Check(address == 0x7000u, "commit texture state");
        events.emplace_back("commit");
    }
    void DrawFilledRectangle(
        std::uint32_t context,
        const std::array<std::uint32_t, 4>& region,
        const std::array<std::uint8_t, 3>& rgb,
        std::uint8_t alpha) override {
        Check(context == 0x6000u &&
                  region == std::array<std::uint32_t, 4>{1u, 2u, 3u, 4u} &&
                  rgb == std::array<std::uint8_t, 3>{0u, 0u, 0u} &&
                  alpha == 0x80u, "filled rectangle arguments");
        events.emplace_back("fill");
    }
    void DrawTexturedRectangle(
        std::uint32_t context,
        const std::array<std::uint32_t, 4>& region,
        std::uint32_t texture_handle,
        const std::array<std::uint8_t, 3>& rgb,
        std::uint8_t alpha,
        bool alpha_mode) override {
        Check(context == 0x6000u &&
                  region == std::array<std::uint32_t, 4>{1u, 2u, 3u, 4u} &&
                  texture_handle == 0x9000u &&
                  rgb == std::array<std::uint8_t, 3>{255u, 255u, 255u} &&
                  alpha == 0x80u &&
                  alpha_mode == expected_alpha_mode,
              "textured rectangle parameters and mode");
        events.emplace_back(alpha_mode ? "texture-alpha" : "texture-plain");
    }
    std::uint32_t DrawMinimapTextures(
        std::uint32_t manager, std::uint32_t context,
        std::uint32_t x, std::uint32_t y,
        std::uint32_t region_address) override {
        Check(manager == 0x8000u && context == 0x6000u &&
                  x == 1u && y == 2u && region_address == 0x7004u,
              "overlay drawing arguments");
        events.emplace_back("overlays");
        return 0x55u;
    }
    std::uint32_t FindObjectAt(
        std::uint32_t x, std::uint32_t y,
        std::uint32_t context) override {
        Check(x == 13u && y == 24u && context == 0x6666u,
              "offset hit-test arguments");
        events.emplace_back("find-object");
        return 0x8888u;
    }
};

void TestSingletonAccess() {
    Check(pal4::MinimapManagerAccess::kGuardAddress == 0x8e3f70u &&
              pal4::MinimapManagerAccess::kInstanceAddress == 0x8e3f6cu,
          "observed singleton globals");
    pal4::MinimapManagerAccessState state{0x80u, 0u};
    AccessRuntime runtime;
    Check(pal4::MinimapManagerAccess::Get(state, runtime) == 0x7000u &&
              state.guard == 0x81u && state.instance == 0x7000u &&
              runtime.events ==
                  std::vector<std::string>{"manager", "register"},
          "first access registers singleton and preserves other guard bits");
    runtime.events.clear();
    Check(pal4::MinimapManagerAccess::Get(state, runtime) == 0x7000u &&
              runtime.events.empty(), "subsequent access returns cache");
    state = {};
    runtime.result = 0u;
    Check(pal4::MinimapManagerAccess::Get(state, runtime) == 0u &&
              state.guard == 1u, "failed registration still sets guard");
}

void TestConstructAndDestroy() {
    ObjectRuntime runtime;
    runtime.state.Bytes().fill(0xa5u);
    Check(pal4::MinimapManagerObject::Create(0x6cu, runtime) == 0x9000u &&
              runtime.events == std::vector<std::string>{
                  "allocate", "base-construct", "access"},
          "factory allocation and base-constructor order");
    const auto& state = runtime.state;
    Check(state.Bytes()[0x14u] == 0x6cu &&
              state.ReadDword(0x4cu) == 1u &&
              state.ReadDword(0x50u) == 2u &&
              state.Bytes()[0x54u] == 1u &&
              state.Bytes()[0x55u] == 0xffu &&
              state.Bytes()[0x13u] == 0xa5u &&
              state.Bytes()[0x56u] == 0xa5u,
          "fixed fields and preserved gaps");
    for (std::uint32_t offset = 0x18u; offset <= 0x48u; offset += 4u) {
        Check(state.ReadDword(offset) == 0u,
              "every observed pointer and counter is cleared");
    }
    runtime.state.WriteDword(0x18u, 0x3333u);
    runtime.state.WriteDword(0x1cu, 0x4444u);
    runtime.state.WriteDword(0x20u, 0x5555u);
    runtime.events.clear();
    pal4::MinimapManagerObject::Destroy(0x9000u, runtime);
    Check(runtime.events == std::vector<std::string>{
              "access", "free:13107", "cleanup-collection", "free:36864"},
          "destructor frees +0x18, clears fields, cleans collection, frees self");
    runtime.events.clear();
    pal4::MinimapManagerObject::Destroy(0u, runtime);
    Check(runtime.events.empty(), "null destroy makes no runtime calls");
}

void TestAllocationAndConstructionFailures() {
    ObjectRuntime runtime;
    runtime.allocated = 0u;
    Check(pal4::MinimapManagerObject::Create(0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "failed allocation skips construction");
    runtime.allocated = 0x9000u;
    runtime.throw_on_construct = true;
    runtime.events.clear();
    try {
        (void)pal4::MinimapManagerObject::Create(0u, runtime);
        Check(false, "construction should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>{
                  "allocate", "base-construct", "free:36864"},
              "factory frees allocated object after constructor exception");
    }
}

void TestReset() {
    ObjectRuntime runtime;
    runtime.state.WriteDword(0x28u, 0x8000u);
    runtime.state.WriteDword(0x24u, 0x8888u);
    runtime.state.WriteDword(0x18u, 0x4000u);
    runtime.state.WriteDword(0x1cu, 0x400cu);
    runtime.words[0x4000u] = 0x5000u;
    runtime.words[0x4004u] = 0u;
    runtime.words[0x4008u] = 0x6000u;
    Check(pal4::MinimapManagerObject::Reset(0x9000u, runtime) == 0x7777u &&
              runtime.state.ReadDword(0x28u) == 0u &&
              runtime.state.ReadDword(0x24u) == 0u &&
              runtime.state.ReadDword(0x1cu) == 0x4000u,
          "reset clears texture, selected grid and list-end fields");
    Check(runtime.events == std::vector<std::string>{
              "access", "reset-texture:32768", "reset-texture:32768",
              "free:32768", "read:16384", "cleanup-grid:20480",
              "free:20480", "read:16388", "read:16392",
              "cleanup-grid:24576", "free:24576", "cleanup-objects"},
          "reset preserves texture double-reset and per-grid destruction order");
    runtime.events.clear();
    Check(pal4::MinimapManagerObject::Reset(0x9000u, runtime) == 0x7777u &&
              runtime.events ==
                  std::vector<std::string>{"access", "cleanup-objects"},
          "empty reset skips texture and grid cleanup");
}

void TestResetObservesChangingListEnd() {
    ObjectRuntime runtime;
    runtime.state.WriteDword(0x18u, 0x4000u);
    runtime.state.WriteDword(0x1cu, 0x4008u);
    runtime.words[0x4000u] = 0x5000u;
    runtime.words[0x4004u] = 0x6000u;
    runtime.shrink_on_grid_cleanup = true;
    Check(pal4::MinimapManagerObject::Reset(0x9000u, runtime) == 0x7777u &&
              runtime.events == std::vector<std::string>{
                  "access", "read:16384", "cleanup-grid:20480",
                  "free:20480", "cleanup-objects"},
          "reset re-reads list end after grid cleanup changes it");
}

void TestTextureSetup() {
    const std::array<std::uint32_t, 4> region{3u, 4u, 12u, 34u};
    ObjectRuntime runtime;
    runtime.state.WriteDword(0x24u, 0x5555u);
    runtime.state.WriteDword(0x28u, 0x6000u);
    runtime.state.WriteDword(0x50u, 2u);
    pal4::MinimapManagerObject::SetupTexture(0x9000u, region, runtime);
    Check(runtime.state.ReadDword(0x28u) == 0x7000u &&
              runtime.state.ReadDword(0x2cu) == 3u &&
              runtime.state.ReadDword(0x30u) == 4u &&
              runtime.state.ReadDword(0x34u) == 12u &&
              runtime.state.ReadDword(0x38u) == 34u,
          "setup records requested region and new texture");
    Check(runtime.events == std::vector<std::string>{
              "access", "grid-pixels", "reset-texture:24576", "free:24576",
              "allocate-texture", "initialize-texture"},
          "old texture released before new allocation and construction");

    runtime.events.clear();
    runtime.state.WriteDword(0x50u, 0u);
    runtime.expected_mode = false;
    runtime.texture_allocated = 0u;
    pal4::MinimapManagerObject::SetupTexture(0x9000u, region, runtime);
    Check(runtime.state.ReadDword(0x28u) == 0u &&
              runtime.events == std::vector<std::string>{
                  "access", "grid-pixels", "reset-texture:28672",
                  "free:28672", "allocate-texture"},
          "failed new allocation clears texture field");

    runtime.events.clear();
    runtime.texture_allocated = 0x7000u;
    runtime.throw_on_texture = true;
    try {
        pal4::MinimapManagerObject::SetupTexture(0x9000u, region, runtime);
        Check(false, "texture constructor exception expected");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>{
                  "access", "grid-pixels", "allocate-texture",
                  "initialize-texture", "free:28672"},
              "constructor exception frees newly allocated texture");
    }
}

void TestTextureSetupGuards() {
    ObjectRuntime runtime;
    const std::array<std::uint32_t, 4> region{3u, 4u, 12u, 34u};
    runtime.state.WriteDword(0x28u, 0x6000u);
    runtime.state.WriteDword(0x2cu, 0x1111u);
    pal4::MinimapManagerObject::SetupTexture(0x9000u, region, runtime);
    Check(runtime.events == std::vector<std::string>{"access"} &&
              runtime.state.ReadDword(0x28u) == 0x6000u &&
              runtime.state.ReadDword(0x2cu) == 0x1111u,
          "missing grid preserves texture and region");
    runtime.events.clear();
    runtime.state.WriteDword(0x24u, 0x5555u);
    runtime.pixels = 0u;
    pal4::MinimapManagerObject::SetupTexture(0x9000u, region, runtime);
    Check(runtime.events ==
              std::vector<std::string>{"access", "grid-pixels"} &&
              runtime.state.ReadDword(0x28u) == 0x6000u,
          "missing grid pixels skips replacement");
}

void TestDraw() {
    DrawRuntime runtime;
    runtime.state.Bytes()[0x54u] = 1u;
    runtime.state.Bytes()[0x55u] = 0x80u;
    runtime.state.WriteDword(0x28u, 0x7000u);
    runtime.state.WriteDword(0x2cu, 1u);
    runtime.state.WriteDword(0x30u, 2u);
    runtime.state.WriteDword(0x34u, 3u);
    runtime.state.WriteDword(0x38u, 4u);
    runtime.state.WriteDword(0x50u, 2u);
    Check(pal4::MinimapManagerObject::Draw(0x8000u, 0x6000u, runtime) ==
              0x55u &&
              runtime.events == std::vector<std::string>{
                  "access", "texture-field:40", "commit", "fill",
                  "texture-field:40", "texture-alpha", "overlays"},
          "alpha drawing commits, fills, textures and overlays in order");
    runtime.events.clear();
    runtime.state.WriteDword(0x50u, 0u);
    runtime.expected_alpha_mode = false;
    Check(pal4::MinimapManagerObject::Draw(0x8000u, 0x6000u, runtime) ==
              0x55u && runtime.events[5] == "texture-plain",
          "other modes use unblended textured rectangle");

    runtime.events.clear();
    runtime.state.Bytes()[0x54u] = 0u;
    Check(pal4::MinimapManagerObject::Draw(0x8000u, 0x6000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"access"},
          "disabled manager skips all drawing");
    runtime.events.clear();
    runtime.state.Bytes()[0x54u] = 1u;
    runtime.state.WriteDword(0x28u, 0u);
    Check(pal4::MinimapManagerObject::Draw(0x8000u, 0x6000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"access"},
          "missing texture skips drawing");
    runtime.events.clear();
    runtime.state.WriteDword(0x28u, 0x7000u);
    runtime.texture = 0u;
    Check(pal4::MinimapManagerObject::Draw(0x8000u, 0x6000u, runtime) ==
              0x7000u &&
              runtime.events ==
                  std::vector<std::string>{"access", "texture-field:40"},
          "missing texture handle skips all rendering helpers");

    pal4::MinimapManagerDrawStub stub;
    Check(pal4::MinimapManagerObject::Draw(0u, 0u, stub) == 0u,
          "drawing stub is linkable");
}

void TestFindObjectWithOffset() {
    DrawRuntime runtime;
    Check(pal4::MinimapManagerObject::FindObjectWithOffset(
              0x8000u, 3u, 4u, 0x6666u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"access"},
          "missing texture returns null before querying offsets");
    runtime.events.clear();
    runtime.state.WriteDword(0x28u, 0x7000u);
    Check(pal4::MinimapManagerObject::FindObjectWithOffset(
              0x8000u, 3u, 4u, 0x6666u, runtime) == 0x8888u &&
              runtime.events == std::vector<std::string>{
                  "access", "texture-field:4", "texture-field:8",
                  "find-object"},
          "texture offsets are added before hit-test call");
}

void TestTextureByIndex() {
    ObjectRuntime runtime;
    runtime.state.WriteDword(0x18u, 0x4000u);
    runtime.state.WriteDword(0x1cu, 0x4008u);
    runtime.state.WriteDword(0x28u, 0x6000u);
    runtime.state.WriteDword(0x34u, 12u);
    runtime.state.WriteDword(0x38u, 34u);
    runtime.state.WriteDword(0x50u, 2u);
    runtime.words[0x4004u] = 0x5555u;
    pal4::MinimapManagerObject::SetTextureByIndex(0x9000u, 1u, runtime);
    Check(runtime.state.ReadDword(0x24u) == 0x5555u &&
              runtime.state.ReadDword(0x28u) == 0x7000u &&
              runtime.events == std::vector<std::string>{
                  "access", "read:16388", "reset-texture:24576",
                  "free:24576", "allocate-texture",
                  "initialize-texture", "init-dialog"},
          "valid index selects grid, replaces texture and initializes dialog");
    runtime.events.clear();
    pal4::MinimapManagerObject::SetTextureByIndex(0x9000u, 2u, runtime);
    Check(runtime.events == std::vector<std::string>{
              "access", "invalid-index:2"} &&
              runtime.state.ReadDword(0x24u) == 0x5555u,
          "out-of-range index logs without replacing texture");
    runtime.events.clear();
    pal4::MinimapManagerObject::SetTextureByIndex(
        0x9000u, 0xffffffffu, runtime);
    Check(runtime.events.empty(), "minus-one index exits before state access");

    runtime.events.clear();
    runtime.texture_allocated = 0u;
    pal4::MinimapManagerObject::SetTextureByIndex(0x9000u, 1u, runtime);
    Check(runtime.state.ReadDword(0x28u) == 0u &&
              runtime.events.back() == "init-dialog",
          "allocation failure still invokes dialog initialization");
}

void TestTextureByIndexEmptyList() {
    ObjectRuntime runtime;
    pal4::MinimapManagerObject::SetTextureByIndex(0x9000u, 0u, runtime);
    Check(runtime.events == std::vector<std::string>{"access"},
          "missing list exits without logging");
    runtime.events.clear();
    runtime.state.WriteDword(0x18u, 0x4000u);
    runtime.state.WriteDword(0x1cu, 0x4000u);
    pal4::MinimapManagerObject::SetTextureByIndex(0x9000u, 0u, runtime);
    Check(runtime.events == std::vector<std::string>{"access"},
          "empty list exits without logging");
}

void TestCurrentGridIndex() {
    ObjectRuntime runtime;
    Check(pal4::MinimapManagerObject::CurrentGridIndex(0x9000u, runtime) ==
              0xffffffffu,
          "missing list has no current grid index");
    runtime.events.clear();
    runtime.state.WriteDword(0x18u, 0x4000u);
    runtime.state.WriteDword(0x1cu, 0x400cu);
    runtime.state.WriteDword(0x24u, 0x5555u);
    runtime.words[0x4000u] = 0x1111u;
    runtime.words[0x4004u] = 0x5555u;
    runtime.words[0x4008u] = 0x5555u;
    Check(pal4::MinimapManagerObject::CurrentGridIndex(0x9000u, runtime) ==
              1u &&
              runtime.events == std::vector<std::string>{
                  "access", "read:16384", "read:16388"},
          "returns first matching grid index");
    runtime.events.clear();
    runtime.state.WriteDword(0x24u, 0x9999u);
    Check(pal4::MinimapManagerObject::CurrentGridIndex(0x9000u, runtime) ==
              0xffffffffu && runtime.events.size() == 4u,
          "missing selected grid returns minus one after full scan");
    runtime.events.clear();
    runtime.shrink_on_read = true;
    runtime.state.WriteDword(0x1cu, 0x400cu);
    Check(pal4::MinimapManagerObject::CurrentGridIndex(0x9000u, runtime) ==
              0xffffffffu && runtime.events ==
                  std::vector<std::string>{"access", "read:16384"},
          "index query observes list shrink after reading first entry");
}

}  // namespace

int main() {
    TestSingletonAccess();
    TestConstructAndDestroy();
    TestAllocationAndConstructionFailures();
    TestReset();
    TestResetObservesChangingListEnd();
    TestTextureSetup();
    TestTextureSetupGuards();
    TestDraw();
    TestFindObjectWithOffset();
    TestTextureByIndex();
    TestTextureByIndexEmptyList();
    TestCurrentGridIndex();
    return EXIT_SUCCESS;
}
