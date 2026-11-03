#include "pal4/strip_system_effect.h"
#include "pal4/strip_resource_block.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct FactoryRuntime final : pal4::IStripSystemFactoryRuntime {
    static constexpr std::uint32_t kEffect = 0x8000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = kEffect;
    std::uint32_t next_head = 0x9000u;
    bool initialized = true;

    std::uint32_t Allocate(std::uint32_t size) override {
        if (size == 0x118u) {
            calls.emplace_back("allocate");
            return allocation;
        }
        Check(size == 0x24u, "each strip block allocates a 36-byte head");
        calls.emplace_back("allocate head");
        const auto head = next_head;
        if (head != 0u) next_head += 0x40u;
        return head;
    }
    std::uint8_t StackByte() override {
        calls.emplace_back("stack byte");
        return 0xa5u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == kEffect)
            calls.emplace_back(value ==
                pal4::StripSystemEffect::kBaseVtable ?
                "base vtable" : "derived vtable");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        if (address >= kEffect + 4u && address <= kEffect + 6u) {
            Check(value == 0u, "constructor clears its state bytes");
            calls.push_back("byte " + std::to_string(address - kEffect));
        }
    }
    bool Initialize(std::uint32_t effect, std::uint32_t config,
                    std::uint32_t clump,
                    std::uint32_t context) override {
        Check(effect == kEffect && config == 0x1000u &&
              clump == 0x5000u && context == 0x3000u &&
              words.at(kEffect) == pal4::StripSystemEffect::kVtable &&
              bytes.at(kEffect + 4u) == 0u &&
              bytes.at(kEffect + 5u) == 0u &&
              bytes.at(kEffect + 6u) == 0u &&
              words.at(words.at(kEffect + 120u)) ==
                  words.at(kEffect + 120u) &&
              words.at(words.at(kEffect + 256u)) ==
                  words.at(kEffect + 256u) &&
              words.at(kEffect + 120u) != words.at(kEffect + 256u),
              "initializer sees the finished strip object and arguments");
        calls.emplace_back("initialize");
        return initialized;
    }
    void Destroy(std::uint32_t effect, bool free_memory) override {
        Check(effect == kEffect && free_memory,
              "failed strip initialization invokes deleting destructor");
        calls.emplace_back("destroy");
    }
};

struct TimeRuntime final : pal4::IStripSystemTimeRuntime {
    std::vector<std::string> calls;
    float ReadFloat(std::uint32_t address) override {
        Check(address == FactoryRuntime::kEffect + 68u,
              "time comes from the effect's +68 field");
        calls.emplace_back("read");
        return 3.25f;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == FactoryRuntime::kEffect + 5u && value == 1u,
              "time query sets the +5 flag");
        calls.emplace_back("mark");
    }
};

struct ConfigRuntime final : pal4::IStripSystemConfigRuntime {
    static constexpr std::uint32_t kEffect = FactoryRuntime::kEffect;
    static constexpr std::uint32_t kConfig = 0x1000u;
    static constexpr std::uint32_t kClump = 0x5000u;
    static constexpr std::uint32_t kContext = 0x3000u;
    std::map<std::string, std::int32_t> ints{{"Hook_Tag", 1}};
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t found_texture = 0u;
    std::uint32_t loaded_texture = 0x7000u;
    bool first_ready = true;
    bool second_ready = true;

    void Reset(std::uint32_t effect) override {
        Check(effect == kEffect, "configuration resets its strip object");
        calls.emplace_back("reset");
    }
    std::int32_t ReadInt(std::uint32_t config,
                         std::string_view key,
                         std::int32_t fallback) override {
        Check(config == kConfig, "integer key comes from the XML node");
        const auto name = std::string{key};
        calls.push_back("int " + name);
        const auto found = ints.find(name);
        return found == ints.end() ? fallback : found->second;
    }
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == kConfig && key == "Attr_StripSystem_Life" &&
              fallback == 0.0f,
              "strip lifetime uses its native float attribute");
        calls.emplace_back("life");
        return 2.5f;
    }
    std::string ReadText(std::uint32_t config,
                         std::string_view key) override {
        Check(config == kConfig &&
              key == "Attr_StripSystem_Texture",
              "strip texture uses its native text attribute");
        calls.emplace_back("texture name");
        return "ribbon.dds";
    }
    std::uint32_t FindTexture(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\ribbon.dds",
              "strip texture lookup uses the native path");
        calls.emplace_back("find texture");
        return found_texture;
    }
    std::uint32_t LoadTexture(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\ribbon.dds",
              "strip texture fallback uses the same path");
        calls.emplace_back("load texture");
        return loaded_texture;
    }
    void DestroyTexture(std::uint32_t texture) override {
        Check(texture == (found_texture != 0u ? found_texture :
                          loaded_texture),
              "strip releases its acquired texture reference");
        calls.emplace_back("destroy texture");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address - kEffect));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address - kEffect));
    }
    void SetDivision(std::uint32_t block,
                     std::uint32_t value) override {
        Check(value == 0u &&
              (block == kEffect + 8u || block == kEffect + 144u),
              "each resource block resets its division property");
        calls.push_back(block == kEffect + 8u ?
                        "first division" : "second division");
    }
    bool SetupBlock(std::uint32_t block, std::uint32_t texture,
                    std::uint32_t clump, float lifetime,
                    std::uint8_t mode) override {
        Check(texture == (found_texture != 0u ? found_texture :
                          loaded_texture) &&
              clump == kClump && lifetime == 2.5f,
              "both setup calls share texture, clump and lifetime");
        if (block == kEffect + 8u) {
            Check(mode == 1u, "first block uses mode one");
            calls.emplace_back("first setup");
            return first_ready;
        }
        Check(block == kEffect + 144u && mode == 0u,
              "second block uses mode zero");
        calls.emplace_back("second setup");
        return second_ready;
    }
};

struct LifecycleRuntime final : pal4::IStripSystemLifecycleRuntime {
    static constexpr std::uint32_t kEffect = FactoryRuntime::kEffect;
    std::map<std::uint32_t, std::uint8_t> bytes{
        {kEffect + 4u, 0u}, {kEffect + 5u, 1u},
        {kEffect + 6u, 0u}};
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("clear " + std::to_string(address - kEffect));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == kEffect, "destructor writes the vtable slot");
        words[address] = value;
        calls.emplace_back(value == pal4::StripSystemEffect::kVtable ?
                           "derived vtable" : "base vtable");
    }
    std::uint8_t CleanupResources(std::uint32_t block) override {
        Check(block == kEffect + 8u || block == kEffect + 144u,
              "cleanup uses the two native resource blocks");
        calls.emplace_back(block == kEffect + 8u ?
                           "first cleanup" : "second cleanup");
        return block == kEffect + 8u ? 0x21u : 0x42u;
    }
    std::uint8_t UpdateVisual(std::uint32_t block, double seconds,
                              std::uint8_t time_flag) override {
        Check((block == kEffect + 8u || block == kEffect + 144u) &&
              seconds == 0.25 && time_flag == 1u,
              "visual update receives elapsed time and time flag");
        calls.emplace_back(block == kEffect + 8u ?
                           "first update" : "second update");
        return block == kEffect + 8u ? 0x33u : 0x77u;
    }
    void ManageVisual(std::uint32_t block) override {
        Check(block == kEffect + 8u || block == kEffect + 144u,
              "visual manager receives a native resource block");
        calls.emplace_back(block == kEffect + 8u ?
                           "first manage" : "second manage");
    }
    void DestructResourceBlocks(std::uint32_t first,
                                std::uint32_t size,
                                std::uint32_t count) override {
        Check(first == kEffect + 8u && size == 0x88u && count == 2u,
              "destructor releases two 136-byte resource blocks");
        calls.emplace_back("destruct blocks");
    }
};

struct BlockRuntime final : pal4::IStripResourceBlockRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next_head = 0x9000u;
    std::uint8_t stack_byte = 0xa5u;

    std::uint8_t StackByte() override {
        calls.emplace_back("stack byte");
        return stack_byte;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x24u, "strip list head uses 36 bytes");
        calls.emplace_back("allocate head");
        const auto head = next_head;
        if (head != 0u) next_head += 0x40u;
        return head;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
};

void TestFactory() {
    FactoryRuntime runtime;
    Check(pal4::StripSystemEffect::Create(
              0x1000u, 0u, 0x3000u, runtime) == 0u &&
          runtime.calls.empty(), "null clump skips strip allocation");
    runtime.allocation = 0u;
    Check(pal4::StripSystemEffect::Create(
              0x1000u, 0x5000u, 0x3000u, runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{"allocate"}),
          "allocation failure returns no strip effect");
    runtime.allocation = FactoryRuntime::kEffect;
    runtime.initialized = false;
    runtime.calls.clear();
    const std::vector<std::string> construction{
        "allocate", "base vtable", "byte 4", "byte 5", "byte 6",
        "stack byte", "allocate head", "stack byte", "allocate head",
        "derived vtable", "initialize"};
    Check(pal4::StripSystemEffect::Create(
              0x1000u, 0x5000u, 0x3000u, runtime) == 0u &&
          runtime.calls.size() == construction.size() + 1u &&
          std::equal(construction.begin(), construction.end(),
                     runtime.calls.begin()) &&
          runtime.calls.back() == "destroy",
          "failed initialization destroys the fully constructed effect");
    runtime.initialized = true;
    runtime.calls.clear();
    Check(pal4::StripSystemEffect::Create(
              0x1000u, 0x5000u, 0x3000u, runtime) ==
              FactoryRuntime::kEffect &&
          runtime.calls == construction,
          "successful factory returns the initialized strip effect");
    FactoryRuntime failed_head;
    failed_head.next_head = 0u;
    Check(pal4::StripSystemEffect::Create(
              0x1000u, 0x5000u, 0x3000u, failed_head) == 0u &&
          failed_head.calls.back() == "destroy",
          "failed block-head allocation destroys the outer effect");
}

void TestTime() {
    TimeRuntime runtime;
    Check(pal4::StripSystemEffect::GetTime(
              FactoryRuntime::kEffect, runtime) == 3.25 &&
          runtime.calls == (std::vector<std::string>{"read", "mark"}),
          "time query reads before marking the strip effect");
}

void TestConfiguration() {
    ConfigRuntime runtime;
    Check(!pal4::StripSystemEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, 0u, ConfigRuntime::kClump,
              ConfigRuntime::kContext, runtime) &&
          runtime.calls == (std::vector<std::string>{"reset"}),
          "missing config stops after reset");
    runtime.calls.clear();
    runtime.ints["Hook_Tag"] = 0;
    Check(!pal4::StripSystemEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kClump, ConfigRuntime::kContext, runtime) &&
          runtime.calls == (std::vector<std::string>{
              "reset", "int Hook_Tag"}),
          "only Hook_Tag one selects StripSystemEffect");
    runtime.ints["Hook_Tag"] = 1;
    runtime.loaded_texture = 0u;
    runtime.calls.clear();
    Check(!pal4::StripSystemEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kClump, ConfigRuntime::kContext, runtime) &&
          runtime.calls.back() == "load texture" &&
          runtime.words.empty() && runtime.bytes.empty(),
          "missing texture leaves both blocks untouched");
    runtime.loaded_texture = 0x7000u;
    runtime.first_ready = false;
    runtime.calls.clear();
    Check(!pal4::StripSystemEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kClump, ConfigRuntime::kContext, runtime) &&
          runtime.calls[runtime.calls.size() - 2u] == "first setup" &&
          runtime.calls.back() == "destroy texture" &&
          runtime.bytes.count(ConfigRuntime::kEffect + 145u) == 0u,
          "first block failure releases texture and skips the second");

    runtime.first_ready = true;
    runtime.second_ready = false;
    runtime.calls.clear();
    Check(pal4::StripSystemEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kClump, ConfigRuntime::kContext, runtime) &&
          runtime.bytes.at(ConfigRuntime::kEffect + 6u) == 0u &&
          runtime.bytes.at(ConfigRuntime::kEffect + 4u) == 1u &&
          runtime.calls.back() == "byte 4",
          "second block failure still activates the first strip");

    runtime.second_ready = true;
    runtime.found_texture = 0x7100u;
    runtime.ints["Attr_StripSystem_AlphaBlend"] = 1;
    runtime.ints["Attr_StripSystem_Color_Red"] = 0x11;
    runtime.ints["Attr_StripSystem_Color_Green"] = 0x22;
    runtime.ints["Attr_StripSystem_Color_Blue"] = 0x33;
    runtime.ints["Attr_StripSystem_Color_Alpha"] = 0x44;
    runtime.calls.clear();
    Check(pal4::StripSystemEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kClump, ConfigRuntime::kContext, runtime) &&
          runtime.words.at(ConfigRuntime::kEffect + 60u) == 0x44332211u &&
          runtime.words.at(ConfigRuntime::kEffect + 196u) == 0x44332211u &&
          runtime.bytes.at(ConfigRuntime::kEffect + 9u) == 1u &&
          runtime.bytes.at(ConfigRuntime::kEffect + 145u) == 1u &&
          runtime.bytes.at(ConfigRuntime::kEffect + 6u) == 1u &&
          runtime.bytes.at(ConfigRuntime::kEffect + 4u) == 1u &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "load texture") == runtime.calls.end() &&
          std::count(runtime.calls.begin(), runtime.calls.end(),
                     "destroy texture") == 1,
          "both blocks share packed RGBA, alpha and one cached texture");
}

void TestLifecycle() {
    LifecycleRuntime runtime;
    Check(pal4::StripSystemEffect::CleanupObject(
              LifecycleRuntime::kEffect, runtime) == 0u &&
          pal4::StripSystemEffect::UpdateVisuals(
              LifecycleRuntime::kEffect, 0.25, runtime) == 0u &&
          runtime.calls.empty(),
          "inactive strip skips resource cleanup and update");
    pal4::StripSystemEffect::ManageVisuals(
        LifecycleRuntime::kEffect, runtime);
    Check(runtime.calls.empty(), "inactive strip skips visual manager");

    runtime.bytes[LifecycleRuntime::kEffect + 4u] = 1u;
    Check(pal4::StripSystemEffect::UpdateVisuals(
              LifecycleRuntime::kEffect, 0.25, runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{"first update"}),
          "single strip updates first block and returns the second flag");
    runtime.calls.clear();
    pal4::StripSystemEffect::ManageVisuals(
        LifecycleRuntime::kEffect, runtime);
    Check(runtime.calls == (std::vector<std::string>{"first manage"}),
          "single strip manages only first visual");

    runtime.bytes[LifecycleRuntime::kEffect + 6u] = 1u;
    runtime.calls.clear();
    Check(pal4::StripSystemEffect::UpdateVisuals(
              LifecycleRuntime::kEffect, 0.25, runtime) == 0x77u &&
          runtime.calls == (std::vector<std::string>{
              "first update", "second update"}),
          "dual strip returns the second visual update result");
    runtime.calls.clear();
    pal4::StripSystemEffect::ManageVisuals(
        LifecycleRuntime::kEffect, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "first manage", "second manage"}),
          "dual strip manages both visuals");

    runtime.calls.clear();
    Check(pal4::StripSystemEffect::CleanupObject(
              LifecycleRuntime::kEffect, runtime) == 0x42u &&
          runtime.bytes.at(LifecycleRuntime::kEffect + 4u) == 0u &&
          runtime.bytes.at(LifecycleRuntime::kEffect + 6u) == 0u &&
          runtime.calls == (std::vector<std::string>{
              "clear 4", "clear 6", "first cleanup", "second cleanup"}),
          "cleanup clears both flags before releasing both resource blocks");

    runtime.bytes[LifecycleRuntime::kEffect + 4u] = 1u;
    runtime.bytes[LifecycleRuntime::kEffect + 6u] = 1u;
    runtime.calls.clear();
    pal4::StripSystemEffect::Destruct(
        LifecycleRuntime::kEffect, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "derived vtable", "clear 4", "clear 6",
              "first cleanup", "second cleanup", "destruct blocks",
              "base vtable"}) &&
          runtime.words.at(LifecycleRuntime::kEffect) ==
              pal4::StripSystemEffect::kBaseVtable,
          "derived destructor cleans blocks before base vtable transition");
}

void TestResourceBlockConstructor() {
    BlockRuntime runtime;
    constexpr auto block = 0x8000u;
    Check(pal4::StripResourceBlock::Construct(block, runtime) == block &&
          runtime.calls == (std::vector<std::string>{
              "stack byte", "allocate head"}) &&
          runtime.bytes.at(block) == 0u &&
          runtime.bytes.at(block + 1u) == 1u &&
          runtime.bytes.at(block + 2u) == 1u &&
          runtime.bytes.at(block + 4u) == 0u &&
          runtime.bytes.at(block + 108u) == 0xa5u &&
          runtime.bytes.at(block + 120u) == 0xa5u,
          "resource constructor writes the native byte fields");
    for (const auto offset : {56u, 60u, 64u, 100u, 104u,
                              116u, 124u, 128u, 132u})
        Check(runtime.words.at(block + offset) == 0u,
              "resource constructor zeros each native word field");
    Check(runtime.words.at(block + 112u) == 0x9000u &&
          runtime.words.at(0x9000u) == 0x9000u &&
          runtime.words.at(0x9004u) == 0x9000u,
          "resource constructor installs a self-linked list head");

    runtime.stack_byte = 0x5au;
    runtime.calls.clear();
    Check(pal4::StripResourceBlock::Construct(
              block + 0x88u, runtime) == block + 0x88u &&
          runtime.bytes.at(block + 0x88u + 108u) == 0x5au &&
          runtime.bytes.at(block + 0x88u + 120u) == 0x5au &&
          runtime.words.at(block + 0x88u + 112u) == 0x9040u,
          "second resource block owns a separate head and stack byte");

    BlockRuntime failed;
    failed.next_head = 0u;
    Check(pal4::StripResourceBlock::Construct(block, failed) == 0u &&
          failed.words.count(block + 112u) == 0u,
          "head allocation failure is reported without a null dereference");
}

}  // namespace

int main() {
    TestFactory();
    TestTime();
    TestConfiguration();
    TestLifecycle();
    TestResourceBlockConstructor();
}
