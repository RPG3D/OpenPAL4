#include "pal4/rw_texture_object.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IRwTextureObjectRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t free_result = 0x55u;
    std::uint32_t clump_result = 0x88u;
    std::uint32_t array_result = 0x900u;
    std::uint32_t next_texture = 0x1000u;
    std::uint32_t world_result = 0;
    std::uint32_t render_result = 0x34u;
    std::uint8_t apply_result = 0x56u;
    std::map<const pal4::StringBufferState*, std::string> texts;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> textures;

    void InitializeEmpty(pal4::StringBufferState& text) override {
        calls.emplace_back("init");
        text = {};
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return free_result;
    }
    std::uint32_t LoadClump(const std::string& path,
                            const std::string& source,
                            std::uint32_t options) override {
        calls.emplace_back("clump:" + path + ":" + source + ":" +
                           std::to_string(options));
        return clump_result;
    }
    void AssignText(pal4::StringBufferState& text,
                    const std::string& value) override {
        calls.emplace_back("assign:" + value);
        texts[&text] = value;
        text.length = static_cast<std::uint32_t>(value.size());
    }
    void PrepareImageName(const std::string& source) override {
        calls.emplace_back("image:" + source);
    }
    std::uint32_t AllocateTextureHandles(std::uint32_t count) override {
        calls.emplace_back("allocate:" + std::to_string(count));
        return array_result;
    }
    std::uint32_t ReadTexture(const std::string& filename) override {
        calls.emplace_back("texture:" + filename);
        return next_texture++;
    }
    void StoreTexture(std::uint32_t array, std::uint32_t index,
                      std::uint32_t texture) override {
        calls.emplace_back("store:" + std::to_string(array) + ":" +
                           std::to_string(index) + ":" +
                           std::to_string(texture));
        textures[{array, index}] = texture;
    }
    std::string ReadText(const pal4::StringBufferState& text) override {
        calls.emplace_back("read-text");
        return texts.at(&text);
    }
    std::uint32_t WorldForClump(std::uint32_t clump) override {
        calls.emplace_back("world:" + std::to_string(clump));
        return world_result;
    }
    void RemoveClump(std::uint32_t world,
                     std::uint32_t clump) override {
        calls.emplace_back("remove-clump:" + std::to_string(world) +
                           ":" + std::to_string(clump));
    }
    void DestroyClump(std::uint32_t clump) override {
        calls.emplace_back("destroy-clump:" + std::to_string(clump));
    }
    std::string NormalizeResourceName(const std::string& path) override {
        calls.emplace_back("normalize:" + path);
        return "normalized:" + path;
    }
    void RemoveTextureByName(std::uint32_t dictionary,
                             const std::string& name) override {
        calls.emplace_back("remove-name:" + std::to_string(dictionary) +
                           ":" + name);
    }
    std::uint32_t TextureAt(std::uint32_t array,
                            std::uint32_t index) override {
        calls.emplace_back("texture-at:" + std::to_string(array) +
                           ":" + std::to_string(index));
        return textures[{array, index}];
    }
    void DestroyTexture(std::uint32_t texture) override {
        calls.emplace_back("destroy-texture:" + std::to_string(texture));
    }
    std::uint32_t RenderClump(std::uint32_t clump) override {
        calls.emplace_back("render:" + std::to_string(clump));
        return render_result;
    }
    std::uint8_t ApplyTexture(std::uint32_t clump,
                              std::uint32_t texture) override {
        calls.emplace_back("apply:" + std::to_string(clump) +
                           ":" + std::to_string(texture));
        return apply_result;
    }
};

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << message << '\n';
    }
}

void TestConstruct() {
    Runtime runtime;
    pal4::RwTextureObjectState state{};
    state.vtable = 7;
    state.active = 1;
    state.frame_rate = 4.0f;
    state.elapsed = 2.0f;
    state.clump = 3;
    state.name = {4, 5, 6};
    state.prefix = {7, 8, 9};
    state.texture_count = 10;
    state.texture_index = 11;
    state.textures = 12;
    auto& result = pal4::RwTextureObject::Construct(state, runtime);
    Check(&result == &state && state.vtable == 0x84b988u,
          "constructor must return state and set vtable");
    Check(state.active == 0 &&
              std::bit_cast<std::uint32_t>(state.frame_rate) == 0x41f00000u &&
              state.elapsed == 0.0f && state.clump == 0,
          "constructor scalar defaults");
    Check(state.name.data == 0 && state.name.length == 0 &&
              state.name.capacity == 0 && state.prefix.data == 0 &&
              state.prefix.length == 0 && state.prefix.capacity == 0 &&
              runtime.calls == std::vector<std::string>({"init", "init"}),
          "constructor initializes both strings in order");
    Check(state.texture_count == 0 && state.texture_index == 0 &&
              state.textures == 0, "constructor texture defaults");
}

void TestDestructSharedStrings() {
    Runtime runtime;
    pal4::RwTextureObjectState state{};
    state.name = {0x101u, 3u, 5u};
    state.prefix = {0x201u, 7u, 9u};
    runtime.bytes[0x100u] = 3u;
    runtime.bytes[0x200u] = 1u;
    const auto result = pal4::RwTextureObject::Destruct(state, runtime);
    Check(result == 0x100u && state.vtable == 0x84b988u,
          "destructor returns name header after shared decrement");
    Check(runtime.bytes[0x100u] == 2u && runtime.bytes[0x200u] == 0u &&
              runtime.calls == std::vector<std::string>({
                  "read:512", "write:512", "read:256", "write:256"}),
          "destructor releases prefix before name");
    Check(state.name.data == 0 && state.name.length == 0 &&
              state.name.capacity == 0 && state.prefix.data == 0 &&
              state.prefix.length == 0 && state.prefix.capacity == 0,
          "destructor clears both string records");
}

void TestDestructOwnedAndEmpty() {
    Runtime runtime;
    pal4::RwTextureObjectState state{};
    state.name.data = 0x301u;
    state.prefix.data = 0x401u;
    runtime.bytes[0x300u] = 0xffu;
    runtime.bytes[0x400u] = 0u;
    Check(pal4::RwTextureObject::Destruct(state, runtime) == 0x55u,
          "destructor forwards final free result");
    Check(runtime.calls == std::vector<std::string>({
              "read:1024", "free:1024", "read:768", "free:768"}),
          "zero and saturated reference headers must be freed");
    runtime.calls.clear();
    Check(pal4::RwTextureObject::Destruct(state, runtime) == 0u &&
              runtime.calls.empty(), "empty destructor must not touch runtime");
}

void TestReadFrames() {
    Runtime runtime;
    pal4::RwTextureObjectState state{};
    const auto result = pal4::RwTextureObject::Read(
        state, "actors/", "hero", 7u, "hero_frame", 0.25f, 3u, runtime);
    Check(result && state.clump == 0x88u && state.active == 1u &&
              state.name.length == 7u && state.prefix.length == 4u,
          "read loads clump and stores both names");
    Check(state.texture_count == 3u && state.elapsed == 0.0f &&
              state.texture_index == 0u && state.textures == 0x900u &&
              state.frame_rate == 4.0f,
          "read initializes frame state");
    Check(runtime.calls == std::vector<std::string>({
              "clump:actors/hero.dff:actors/:7",
              "assign:hero", "assign:actors/", "image:actors/",
              "allocate:3", "texture:hero_frame01.png",
              "store:2304:0:4096", "texture:hero_frame02.png",
              "store:2304:1:4097", "texture:hero_frame03.png",
              "store:2304:2:4098"}),
          "read preserves clump, assignment and numbered texture order");
}

void TestReadFailureGates() {
    Runtime runtime;
    pal4::RwTextureObjectState state{};
    runtime.clump_result = 0;
    Check(!pal4::RwTextureObject::Read(
              state, "a", "b", 2u, "f", 1.0f, 2u, runtime) &&
              state.clump == 0u && state.active == 0u &&
              runtime.calls.size() == 1u,
          "failed clump stops before string and texture setup");
    runtime.calls.clear();
    runtime.clump_result = 0x88u;
    Check(!pal4::RwTextureObject::Read(
              state, "a", "b", 2u, nullptr, 1.0f, 2u, runtime) &&
              state.clump == 0x88u && state.active == 0u &&
              runtime.calls.size() == 3u,
          "missing frame prefix stops after clump and names");
    runtime.calls.clear();
    runtime.array_result = 0;
    Check(!pal4::RwTextureObject::Read(
              state, "a", "b", 2u, "f", 1.0f, 2u, runtime) &&
              state.textures == 0u && state.active == 0u &&
              runtime.calls.size() == 5u,
          "failed handle allocation prevents texture writes");
    runtime.calls.clear();
    Check(!pal4::RwTextureObject::Read(
              state, nullptr, "b", 2u, "f", 1.0f, 2u, runtime) &&
              runtime.calls.empty(), "null source is rejected safely");
    Check(!pal4::RwTextureObject::Read(
              state, "a", "b", 2u, "f", 0.0f, 2u, runtime) &&
              runtime.calls.size() == 3u,
          "zero duration stops before division and allocation");
    runtime.calls.clear();
    Check(!pal4::RwTextureObject::Read(
              state, "a", "b", 2u, "f", 1.0f, 0u, runtime) &&
              runtime.calls.size() == 3u,
          "zero frame count stops after names");
}

void TestCleanup() {
    Runtime runtime;
    pal4::RwTextureObjectState state{};
    state.active = 1;
    state.clump = 0x88u;
    state.frame_rate = 4.0f;
    state.elapsed = 1.5f;
    state.texture_count = 3u;
    state.texture_index = 2u;
    state.textures = 0x900u;
    runtime.world_result = 0x22u;
    runtime.AssignText(state.name, "actors/");
    runtime.AssignText(state.prefix, "hero");
    runtime.textures[{0x900u, 0u}] = 0x1000u;
    runtime.textures[{0x900u, 1u}] = 0u;
    runtime.textures[{0x900u, 2u}] = 0x1002u;
    runtime.calls.clear();

    Check(pal4::RwTextureObject::Cleanup(state, 77u, runtime),
          "active cleanup must run");
    Check(runtime.calls == std::vector<std::string>({
              "world:136", "world:136", "remove-clump:34:136",
              "destroy-clump:136", "read-text", "read-text",
              "normalize:actors/hero.dff",
              "remove-name:77:normalized:actors/hero.dff",
              "assign:", "assign:", "texture-at:2304:0",
              "destroy-texture:4096", "texture-at:2304:1",
              "texture-at:2304:2", "destroy-texture:4098",
              "free:2304"}),
          "cleanup preserves world, dictionary, string and texture order");
    Check(state.active == 0 && state.clump == 0 && state.textures == 0 &&
              state.frame_rate == 0.0f && state.elapsed == 0.0f &&
              state.texture_count == 0 && state.texture_index == 0 &&
              state.name.length == 0 && state.prefix.length == 0,
          "cleanup resets runtime state");
    runtime.calls.clear();
    Check(!pal4::RwTextureObject::Cleanup(state, 77u, runtime) &&
              runtime.calls.empty(), "inactive cleanup is a no-op");
}

void TestAdvanceAndTexture() {
    Runtime runtime;
    pal4::RwTextureObjectState state{};
    state.frame_rate = 1.0f;
    state.texture_count = 2u;
    state.textures = 0x900u;
    state.clump = 0x88u;
    runtime.textures[{0x900u, 0u}] = 0x1000u;
    runtime.textures[{0x900u, 1u}] = 0x1001u;
    Check(pal4::RwTextureObject::Advance(state, 2.0f, runtime) == 0 &&
              state.elapsed == 0.0f && runtime.calls.empty(),
          "inactive advance leaves time unchanged");
    state.active = 1;
    Check(pal4::RwTextureObject::Advance(state, 0.4f, runtime) == 0 &&
              state.elapsed == 0.4f && runtime.calls.empty(),
          "subthreshold advance accumulates elapsed time");
    Check(pal4::RwTextureObject::Advance(state, 0.6f, runtime) == 0x56u &&
              state.elapsed == 0.0f && state.texture_index == 1u &&
              runtime.calls == std::vector<std::string>({
                  "texture-at:2304:1", "apply:136:4097"}),
          "threshold advance applies next texture");
    runtime.calls.clear();
    Check(pal4::RwTextureObject::Advance(state, 2.0f, runtime) == 0x56u &&
              state.texture_index == 0u && state.elapsed == 0.0f &&
              runtime.calls == std::vector<std::string>({
                  "texture-at:2304:0", "apply:136:4096"}),
          "advance wraps once and discards excess time");
    runtime.calls.clear();
    Check(pal4::RwTextureObject::Advance(
              state, std::numeric_limits<float>::quiet_NaN(),
              runtime) == 0 && runtime.calls.empty(),
          "unordered elapsed value does not advance frame");
}

void TestRender() {
    Runtime runtime;
    pal4::RwTextureObjectState state{};
    state.clump = 0x88u;
    Check(pal4::RwTextureObject::Render(state, runtime) == 0 &&
              runtime.calls.empty(), "inactive render is a no-op");
    state.active = 1;
    Check(pal4::RwTextureObject::Render(state, runtime) == 0x34u &&
              runtime.calls == std::vector<std::string>({
                  "world:136", "render:136"}),
          "unattached clump renders");
    runtime.calls.clear();
    runtime.world_result = 0x1234u;
    Check(pal4::RwTextureObject::Render(state, runtime) == 0x34u &&
              runtime.calls == std::vector<std::string>({"world:136"}),
          "attached clump suppresses render and forwards low byte");
    runtime.calls.clear();
    Check(pal4::RwTextureObject::ApplyCurrentTexture(state, runtime) == 0 &&
              runtime.calls.empty(), "missing texture array is guarded");
}

}  // namespace

int main() {
    TestConstruct();
    TestDestructSharedStrings();
    TestDestructOwnedAndEmpty();
    TestReadFrames();
    TestReadFailureGates();
    TestCleanup();
    TestAdvanceAndTexture();
    TestRender();
    return failures == 0 ? 0 : 1;
}
