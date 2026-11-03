#include "pal4/dynamic_strip_object_config.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IDynamicStripVisualRuntime {
    std::vector<std::string> calls;
    std::vector<float> endpoint_x;
    std::uint32_t next_frame = 0x100u;

    void CleanupResources(pal4::DynamicStripObjectState&) override {
        calls.emplace_back("cleanup");
    }
    void TextureSize(std::uint32_t texture, std::uint32_t& width,
                     std::uint32_t& height) override {
        Check(texture == 0x500u, "texture size uses supplied texture");
        calls.emplace_back("texture_size");
        width = 100u;
        height = 50u;
    }
    void AddTextureReference(std::uint32_t texture) override {
        Check(texture == 0x500u, "increments supplied texture reference");
        calls.emplace_back("texture_reference");
    }
    std::uint32_t CreateFrame() override {
        calls.emplace_back("create_frame");
        return next_frame++;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x100u || frame == 0x101u,
              "only endpoint frames receive identity");
        calls.emplace_back("identity");
    }
    void ScaleFrame(std::uint32_t, pal4::DynamicStripVector scale,
                    int combine) override {
        Check(scale.x == 1.0f && scale.y == 1.0f &&
                  scale.z == 1.0f && combine == 0,
              "endpoint scale is unit with replace combine");
        calls.emplace_back("scale");
    }
    void RotateFrame(std::uint32_t, pal4::DynamicStripVector axis,
                     float angle, int combine) override {
        Check(angle == 0.0f && combine == 2,
              "endpoint rotation uses zero degrees and post-combine");
        const char name = axis.x == 1.0f ? 'x' :
                          axis.y == 1.0f ? 'y' : 'z';
        Check((name == 'x' && axis.y == 0.0f && axis.z == 0.0f) ||
                  (name == 'y' && axis.x == 0.0f && axis.z == 0.0f) ||
                  (name == 'z' && axis.x == 0.0f && axis.y == 0.0f &&
                   axis.z == 1.0f),
              "rotation axis is unit X Y or Z");
        calls.push_back(std::string{"rotate_"} + name);
    }
    void TranslateFrame(std::uint32_t, pal4::DynamicStripVector position,
                        int combine) override {
        Check(position.y == 0.0f && position.z == 0.0f && combine == 2,
              "endpoint translation is X only");
        calls.emplace_back("translate");
        endpoint_x.push_back(position.x);
    }
    void UpdateFrameObjects(std::uint32_t) override {
        calls.emplace_back("update");
    }
    void AddChildFrame(std::uint32_t parent,
                       std::uint32_t child) override {
        Check(parent == 0x102u &&
                  (child == 0x100u || child == 0x101u),
              "parent contains both endpoint frames");
        calls.emplace_back("add_child");
    }
    void UpdateFrameProperties(
        pal4::DynamicStripObjectState& state) override {
        Check(state.words[14u] == 0x102u &&
                  state.words[0u] == 0x0001ff01u,
              "frame update observes final handles and flags");
        calls.emplace_back("frame_properties");
    }
};

struct HookRuntime final : pal4::IDynamicStripHookRuntime, pal4::IStripVisualBlockHookRuntime {
    std::vector<std::string> calls;
    bool missing_first{};
    bool missing_second{};
    std::uint32_t lookup_count{};

    void CleanupResources(pal4::DynamicStripObjectState&) override {
        calls.emplace_back("cleanup");
    }
    void CleanupResources(pal4::StripVisualBlockView) override {
        calls.emplace_back("cleanup");
    }
    std::uint32_t FindHook(std::uint32_t clump,
                           std::string_view name) override {
        Check(clump == 0x600u, "hook lookup uses input clump");
        calls.push_back(std::string{name});
        ++lookup_count;
        return ((lookup_count == 1u && missing_first) ||
                (lookup_count == 2u && missing_second))
            ? 0u : 0x700u + lookup_count;
    }
    std::uint32_t HookFrame(std::uint32_t hook) override {
        calls.emplace_back("hook_frame");
        return hook + 0x100u;
    }
    void TextureSize(std::uint32_t texture, std::uint32_t& width,
                     std::uint32_t& height) override {
        Check(texture == 0x500u, "hook texture size handle");
        calls.emplace_back("texture_size");
        width = 200u;
        height = 100u;
    }
    void AddTextureReference(std::uint32_t texture) override {
        Check(texture == 0x500u, "hook texture reference handle");
        calls.emplace_back("texture_reference");
    }
};

struct CleanupRuntime final : pal4::IDynamicStripCleanupRuntime {
    std::vector<std::string> calls;
    std::uint32_t first_node = 0x701u;

    std::uint32_t FirstNode(std::uint32_t sentinel) override {
        Check(sentinel == 0x700u, "list sentinel offset 112");
        calls.emplace_back("first");
        return first_node;
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        calls.push_back("next:" + std::to_string(node));
        return node == 0x701u ? 0x702u : 0x700u;
    }
    void UnlinkNode(std::uint32_t node) override {
        calls.push_back("unlink:" + std::to_string(node));
    }
    void FreeNode(std::uint32_t node) override {
        calls.push_back("free:" + std::to_string(node));
    }
    std::uint32_t DestroyTexture(std::uint32_t texture) override {
        Check(texture == 0x500u, "destroys owned texture");
        calls.emplace_back("texture");
        return 0x55u;
    }
    std::uint32_t DestroyFrame(std::uint32_t frame) override {
        calls.push_back("frame:" + std::to_string(frame));
        return frame + 0x900u;
    }
};

void Cleanup() {
    pal4::DynamicStripObjectState inactive;
    inactive.words[16u] = 0x500u;
    CleanupRuntime idle_runtime;
    Check(pal4::DynamicStripObjectConfiguration::CleanupVisual(
              inactive, idle_runtime) == 0u &&
              idle_runtime.calls.empty() && inactive.words[16u] == 0x500u,
          "inactive cleanup makes no calls or changes");

    pal4::DynamicStripObjectState state;
    state.words[0u] = 0x00010001u;
    state.words[14u] = 0x102u;
    state.words[15u] = std::bit_cast<std::uint32_t>(3.5f);
    state.words[16u] = 0x500u;
    state.words[25u] = 0x100u;
    state.words[26u] = 0x101u;
    state.words[28u] = 0x700u;
    state.words[29u] = 2u;
    CleanupRuntime runtime;
    Check(pal4::DynamicStripObjectConfiguration::CleanupVisual(
              state, runtime) == 0xa02u,
          "owned parent frame destruction supplies final return");
    Check(state.words[0u] == 0u && state.words[14u] == 0u &&
              state.words[15u] == 0u && state.words[16u] == 0u &&
              state.words[25u] == 0u && state.words[26u] == 0u &&
              state.words[29u] == 0u,
          "owned cleanup clears flags, resources and list count");
    Check(runtime.calls == std::vector<std::string>{
        "first", "next:1793", "unlink:1793", "free:1793",
        "next:1794", "unlink:1794", "free:1794",
        "texture", "frame:256", "frame:257", "frame:258"},
        "owned cleanup follows list, texture and frame order");

    pal4::DynamicStripObjectState hooked;
    hooked.words[0u] = 0x01010001u;
    hooked.words[14u] = 0x102u;
    hooked.words[16u] = 0x500u;
    hooked.words[25u] = 0x100u;
    hooked.words[26u] = 0x101u;
    hooked.words[28u] = 0x700u;
    CleanupRuntime hook_runtime;
    hook_runtime.first_node = 0x700u;
    Check(pal4::DynamicStripObjectConfiguration::CleanupVisual(
              hooked, hook_runtime) == 0x55u,
          "hook mode returns texture destruction result");
    Check(hooked.words[0u] == 0u && hooked.words[14u] == 0u &&
              hooked.words[25u] == 0x100u &&
              hooked.words[26u] == 0u,
          "hook cleanup preserves external first frame");
    Check(hook_runtime.calls ==
              std::vector<std::string>{"first", "texture"},
          "hook cleanup destroys no external frames");
    Check(pal4::DynamicStripObjectConfiguration::CleanupVisual(
              hooked, hook_runtime) == 0u &&
              hook_runtime.calls.size() == 2u,
          "repeated cleanup is inactive");
}

void HookBlade() {
    for (bool alternate : {false, true}) {
        HookRuntime runtime;
        pal4::DynamicStripObjectState state;
        state.words[0u] = 0xffffffffu;
        Check(pal4::DynamicStripObjectConfiguration::VisualSetupHookBlade(
                  state, 0x500u, 0x600u, 3.5f, alternate, runtime) == 1u,
              "both hook pairs can activate visual");
        Check(runtime.calls == std::vector<std::string>{
            "cleanup", alternate ? "hook_blade_a" : "hook_blade_c",
            alternate ? "hook_blade_b" : "hook_blade_d",
            "hook_frame", "hook_frame", "texture_size",
            "texture_reference"},
            "hook setup follows native lookup and resource order");
        Check(state.words[14u] == 0u && state.words[25u] == 0x801u &&
                  state.words[26u] == 0x802u &&
                  state.words[16u] == 0x500u &&
                  std::bit_cast<float>(state.words[15u]) == 3.5f &&
                  std::bit_cast<float>(state.words[17u]) == 0.0025f &&
                  std::bit_cast<float>(state.words[18u]) == 0.005f &&
                  state.words[0u] == 0x0101ff01u,
              "hook setup stores frames, texture and active flags");
    }
    for (const auto [texture, clump, life] :
         {std::tuple<std::uint32_t, std::uint32_t, float>{
              0u, 0x600u, 3.5f},
          {0x500u, 0u, 3.5f}, {0x500u, 0x600u, 0.0f},
          {0x500u, 0x600u, std::nanf("")}}) {
        HookRuntime runtime;
        pal4::DynamicStripObjectState state;
        Check(pal4::DynamicStripObjectConfiguration::VisualSetupHookBlade(
                  state, texture, clump, life, false, runtime) == 0u &&
                  runtime.calls == std::vector<std::string>{"cleanup"},
              "invalid hook setup stops after cleanup");
    }
    for (bool missing_first : {false, true}) {
        HookRuntime runtime;
        runtime.missing_first = missing_first;
        runtime.missing_second = !missing_first;
        pal4::DynamicStripObjectState state;
        Check(pal4::DynamicStripObjectConfiguration::VisualSetupHookBlade(
                  state, 0x500u, 0x600u, 3.5f, false, runtime) == 0u,
              "either missing hook rejects visual setup");
        Check(runtime.calls == std::vector<std::string>{
            "cleanup", "hook_blade_c", "hook_blade_d"},
            "both hook names are queried before rejection");
        Check(state.words[16u] == 0u && state.words[0u] == 0u,
              "missing hooks leave visual inactive");
    }
}

void Valid() {
    Runtime runtime;
    pal4::DynamicStripObjectState state;
    state.words[0u] = 0xffffffffu;
    Check(pal4::DynamicStripObjectConfiguration::VisualSetup(
              state, 0x500u, 8.0f, 2.5f, runtime) == 1u,
          "valid visual setup returns one");
    Check(state.words[16u] == 0x500u &&
              std::bit_cast<float>(state.words[15u]) == 2.5f &&
              std::bit_cast<float>(state.words[17u]) == 0.005f &&
              std::bit_cast<float>(state.words[18u]) == 0.01f,
          "stores texture, lifetime and half-texel offsets");
    Check(state.words[25u] == 0x100u && state.words[26u] == 0x101u &&
              state.words[14u] == 0x102u,
          "stores endpoint and parent frames");
    Check(runtime.endpoint_x == std::vector<float>{-4.0f, 4.0f},
          "endpoint frames straddle origin");
    Check(runtime.calls == std::vector<std::string>{
        "cleanup", "texture_size", "texture_reference",
        "create_frame", "identity", "scale", "rotate_x", "rotate_y",
        "rotate_z", "translate", "update",
        "create_frame", "identity", "scale", "rotate_x", "rotate_y",
        "rotate_z", "translate", "update",
        "create_frame", "add_child", "add_child", "frame_properties"},
        "native visual resource call order");
}

void Invalid() {
    for (const auto [texture, width, life] :
         {std::tuple<std::uint32_t, float, float>{0u, 8.0f, 2.0f},
          {0x500u, 0.0f, 2.0f}, {0x500u, -1.0f, 2.0f},
          {0x500u, 8.0f, 0.0f}, {0x500u, 8.0f, -1.0f},
          {0x500u, std::nanf(""), 2.0f}}) {
        Runtime runtime;
        pal4::DynamicStripObjectState state;
        Check(pal4::DynamicStripObjectConfiguration::VisualSetup(
                  state, texture, width, life, runtime) == 0u,
              "invalid setup returns zero");
        Check(runtime.calls == std::vector<std::string>{"cleanup"},
              "invalid setup only cleans old resources");
    }
}

void EmbeddedBlockBoundary() {
    struct Guarded {
        std::uint32_t before = 0x12345678u;
        std::array<std::uint32_t, 34u> words{};
        std::uint32_t after = 0x87654321u;
    } state;
    static_assert(sizeof(state.words) == 136u);
    const pal4::StripVisualBlockView view{state.words};
    HookRuntime hook;
    Check(pal4::DynamicStripObjectConfiguration::VisualSetupHookBlade(
              view, 0x500u, 0x600u, 2.0f, true, hook) == 1u,
          "136-byte embedded block accepts hook setup");
    Check(state.words[25u] == 0x801u && state.words[26u] == 0x802u &&
              state.words[16u] == 0x500u && hook.calls.front() == "cleanup",
          "embedded block stores borrowed hook frames and owned texture");
    state.words[28u] = 0x700u;
    state.words[29u] = 2u;
    CleanupRuntime cleanup;
    (void)pal4::DynamicStripObjectConfiguration::CleanupVisual(view, cleanup);
    Check(state.words[29u] == 0u && state.words[16u] == 0u &&
              state.words[26u] == 0u && state.words[25u] == 0x801u,
          "embedded cleanup retires nodes and retains borrowed first frame residue");
    Check(state.before == 0x12345678u && state.after == 0x87654321u,
          "embedded setup and cleanup preserve adjacent resource blocks");
}

}  // namespace

int main() {
    Valid();
    Invalid();
    HookBlade();
    EmbeddedBlockBoundary();
    Cleanup();
    std::cout << "dynamic strip visual setup tests passed\n";
}
