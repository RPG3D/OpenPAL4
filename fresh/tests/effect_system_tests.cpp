#include "pal4/effect_system.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IEffectSystemCleanupRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kState = 0x2000u;
    static constexpr std::uint32_t kHead = 0x3000u;
    std::uint8_t active = 1u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kSystem + 4u, kHead}, {kHead, kHead}};
    std::map<std::uint32_t, std::uint32_t> next;
    std::vector<std::string> calls;

    std::uint32_t InitializeSystem() override {
        calls.push_back("initialize");
        return kState;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kState, "cleanup checks initialization flag");
        return active;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void DestroyXmlNode(std::uint32_t node) override {
        calls.push_back("destroy " + std::to_string(node));
    }
    void Free(std::uint32_t pointer) override {
        calls.push_back("free " + std::to_string(pointer));
    }
    std::uint32_t RemoveInstance(std::uint32_t tree,
                                 std::uint32_t node) override {
        Check(tree == kSystem, "cleanup erases from its own instance tree");
        calls.push_back("erase " + std::to_string(node));
        words[kHead] = next.at(node);
        return next.at(node);
    }
};

struct LoadRuntime final : pal4::IEffectSystemLoadRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kState = 0x2000u;
    static constexpr std::uint32_t kHead = 0x3000u;
    static constexpr std::uint32_t kCached = 0x4000u;
    static constexpr std::uint32_t kXml = 0x5000u;
    static constexpr std::uint32_t kInstance = 0x6000u;
    static constexpr std::uint32_t kFile = 0x7000u;
    static constexpr std::uint32_t kAdapter = 0x8000u;
    bool active = true;
    bool cached = false;
    bool missing_file = false;
    bool parse_ok = true;
    bool load_ok = true;
    bool throw_on_insert = false;
    bool throw_on_load = false;
    std::map<std::uint32_t, std::uint32_t> words{
        {kSystem + 4u, kHead}, {kCached + 28u, kXml}};
    std::vector<std::string> calls;

    std::uint32_t InitializeSystem() override {
        calls.push_back("initialize");
        return kState;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kState, "load checks effect-system state");
        return active ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t FindCached(std::uint32_t system,
                             std::string_view name) override {
        Check(system == kSystem && name == "spark",
              "effect cache lookup uses the requested name");
        calls.push_back("find");
        return cached ? kCached : kHead;
    }
    std::uint32_t CreateInstance() override {
        calls.push_back("create instance");
        return kInstance;
    }
    bool LoadInstance(std::uint32_t instance,
                      std::uint32_t xml) override {
        Check(instance == kInstance && xml == kXml,
              "effect instance loads cached or parsed XML");
        calls.push_back("load instance");
        if (throw_on_load) throw std::runtime_error("instance load failed");
        return load_ok;
    }
    void DestroyInstance(std::uint32_t system,
                         std::uint32_t instance) override {
        Check(system == kSystem && instance == kInstance,
              "failed instance is destroyed in its effect system");
        calls.push_back("destroy instance");
    }
    void BeginScriptScope() override { calls.push_back("begin scripts"); }
    void EndScriptScope() override { calls.push_back("end scripts"); }
    std::string_view EffectDirectory() override {
        calls.push_back("directory");
        return "effects/";
    }
    std::uint32_t OpenFile(std::string_view path) override {
        Check(path == "effects/spark.eft", "effect path appends .eft");
        calls.push_back("open");
        return missing_file ? 0u : kFile;
    }
    void CloseFile(std::uint32_t file) override {
        Check(file == kFile, "effect file handle closes once");
        calls.push_back("close");
    }
    std::uint32_t CreateXmlDocument(std::uint32_t bytes) override {
        Check(bytes == 60u, "native XML document uses 60 bytes");
        calls.push_back("create xml");
        return kXml;
    }
    std::uint32_t CreateFileAdapter(std::uint32_t file) override {
        Check(file == kFile, "XML adapter receives opened file");
        calls.push_back("create adapter");
        return kAdapter;
    }
    bool ParseXml(std::uint32_t adapter, std::uint32_t xml) override {
        Check(adapter == kAdapter && xml == kXml,
              "parser receives file adapter and document");
        calls.push_back("parse");
        return parse_ok;
    }
    void DestroyFileAdapter(std::uint32_t adapter) override {
        Check(adapter == kAdapter, "file adapter is reset");
        calls.push_back("destroy adapter");
    }
    void InsertCached(std::uint32_t system,
                      std::string_view name, std::uint32_t xml) override {
        Check(system == kSystem && name == "spark" && xml == kXml,
              "parsed document is inserted under its name");
        calls.push_back("insert");
        if (throw_on_insert) throw std::runtime_error("insert failed");
    }
    void DestroyXmlNode(std::uint32_t xml) override {
        Check(xml == kXml, "failed parse releases its document");
        calls.push_back("destroy xml");
    }
    void FreeXml(std::uint32_t xml) override {
        Check(xml == kXml, "failed parse frees its document");
        calls.push_back("free xml");
    }
};

struct TreeRuntime final : pal4::IEffectSystemTreeRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kNil = 0x8f605cu;
    static constexpr std::uint32_t kKey = 0x3000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kHead + 4u, kNil}};
    std::map<std::uint32_t, std::string> texts;

    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::int32_t CompareText(std::uint32_t node_text,
                             std::uint32_t key) override {
        const auto& a = texts.at(node_text);
        const auto& b = texts.at(key);
        return a < b ? -1 : (a > b ? 1 : 0);
    }
};

struct InitRuntime final : pal4::IEffectSystemInitRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t list_count = 0u;

    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == kSystem) calls.push_back("vtable");
    }
    void InitializeFirstTree(std::uint32_t address) override {
        Check(address == kSystem + 12u,
              "first effect-system tree starts at +12");
        calls.push_back("first tree");
    }
    std::uint8_t InitializeSecondTree(
        std::uint32_t address) override {
        Check(address == kSystem + 28u,
              "second effect-system tree starts at +28");
        calls.push_back("second tree");
        return 0xa5u;
    }
    std::uint32_t CreateListHead() override {
        calls.push_back("list");
        return 0x9000u + list_count++ * 0x20u;
    }
    void InitializeInlineState(std::uint32_t address) override {
        Check(address == kSystem + 212u &&
              bytes.at(address) == 0xa5u &&
              bytes.at(address + 1u) == 0xa5u &&
              bytes.at(kSystem + 220u) == 0u,
              "inline state receives both status bytes and cleared field");
        calls.push_back("inline state");
    }
    void InitializeFourthTree(std::uint32_t address) override {
        Check(address == kSystem + 228u,
              "fourth effect-system tree starts at +228");
        calls.push_back("fourth tree");
    }
    void InitializeFifthTree(std::uint32_t address) override {
        Check(address == kSystem + 244u,
              "fifth effect-system tree starts at +244");
        calls.push_back("fifth tree");
    }
};

struct EmbeddedRuntime final : pal4::IEffectSystemEmbeddedTreeRuntime {
    using Kind = pal4::EffectSystemEmbeddedTreeKind;
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, 0u}, {kHead + 8u, kHead}};
    std::map<Kind, std::uint32_t> nils{
        {Kind::First, 0x93cdb8u}, {Kind::Second, 0x93cdc0u},
        {Kind::Third, 0x93cdc8u}, {Kind::Fourth, 0x93cdd0u},
        {Kind::Fifth, 0x93cdd8u}};
    std::map<Kind, std::uint32_t> references{
        {Kind::First, 2u}, {Kind::Second, 2u},
        {Kind::Third, 2u}, {Kind::Fourth, 2u}, {Kind::Fifth, 2u}};
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> calls;

    static const char* Name(Kind kind) {
        switch (kind) {
        case Kind::First: return "first";
        case Kind::Second: return "second";
        case Kind::Third: return "third";
        case Kind::Fourth: return "fourth";
        case Kind::Fifth: return "fifth";
        }
        return "unknown";
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t SharedNil(Kind kind) override {
        return nils.at(kind);
    }
    void ClearSharedNil(Kind kind) override {
        calls.push_back(std::string("clear ") + Name(kind));
        nils[kind] = 0u;
    }
    std::uint32_t ReleaseSharedNilReference(Kind kind) override {
        calls.push_back(std::string("release ") + Name(kind));
        return --references[kind];
    }
    void DeleteSubtree(Kind kind, std::uint32_t root) override {
        calls.push_back(std::string("delete ") + Name(kind) + " " +
                        std::to_string(root));
    }
    void ReadFirstElement(std::uint32_t tree) override {
        Check(tree == kTree && words.at(kHead) == kHead &&
              words.at(kHead + 8u) == kHead &&
              words.at(kTree + 12u) == 0u,
              "first-element read follows complete header reset");
        calls.push_back("first element");
    }
    std::uint32_t Next(Kind kind, std::uint32_t node) override {
        calls.push_back(std::string("next ") + Name(kind) + " " +
                        std::to_string(node));
        return successors.at(node);
    }
    void RemoveNode(Kind kind, std::uint32_t tree,
                    std::uint32_t node) override {
        Check(tree == kTree, "embedded tree removes from its own header");
        calls.push_back(std::string("remove ") + Name(kind) + " " +
                        std::to_string(node));
        words[kHead] = successors.at(node);
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        calls.push_back("free " + std::to_string(pointer));
        return pointer + 1u;
    }
};

struct DestroyRuntime final : pal4::IEffectSystemDestroyRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes{{kSystem + 4u, 0u}};
    std::vector<std::string> calls;

    DestroyRuntime() {
        constexpr std::uint32_t offsets[] = {
            200u, 188u, 176u, 164u, 152u, 140u, 128u, 116u};
        for (const auto offset : offsets) {
            const auto head = 0x9000u + offset;
            words[kSystem + offset + 4u] = head;
            words[kSystem + offset + 8u] = 1u;
            words[head] = 0xa000u + offset;
        }
        words[kSystem + 56u] = 0xb000u;
        words[kSystem + 60u] = 0xb020u;
        words[kSystem + 64u] = 0xb040u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u,
              "destructor checks enabled state before teardown");
        calls.push_back("disable check");
        return bytes.at(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == kSystem) calls.push_back("vtable");
    }
    void CleanupEffects(std::uint32_t) override {
        Check(false, "disabled destructor has no live effects");
    }
    void InitializeGeometryData(std::uint32_t) override {
        Check(false, "disabled destructor skips geometry reset");
    }
    void RemoveInlineEffects(std::uint32_t, std::uint32_t,
                             std::uint32_t) override {
        Check(false, "disabled destructor skips inline effects");
    }
    std::uint32_t InitializeSystem() override {
        Check(false, "disabled destructor skips global registration");
        return 0u;
    }
    void RemoveEffect(std::uint32_t, std::uint32_t) override {
        Check(false, "disabled destructor skips manager removal");
    }
    void CleanupTrailingTree(std::uint32_t address) override {
        calls.push_back("tail " + std::to_string(address - kSystem));
    }
    std::uint32_t FirstListElement(std::uint32_t list) override {
        const auto offset = list - kSystem;
        calls.push_back("first " + std::to_string(offset));
        return words.at(words.at(list + 4u));
    }
    void RemoveListElements(std::uint32_t list,
                            std::uint32_t first,
                            std::uint32_t head) override {
        const auto offset = list - kSystem;
        Check(head == 0x9000u + offset &&
              first == 0xa000u + offset,
              "list removal gets first element and its head");
        calls.push_back("remove " + std::to_string(offset));
    }
    void Free(std::uint32_t pointer) override {
        calls.push_back("free " + std::to_string(pointer));
    }
    void CleanupConfigList(std::uint32_t list) override {
        calls.push_back("config " + std::to_string(list - kSystem));
    }
    void ReleaseRange(std::uint32_t first,
                      std::uint32_t last) override {
        Check(first == 0xb000u && last == 0xb020u,
              "range cleanup receives +56 and +60 pointers");
        calls.push_back("range");
    }
    void CleanupSecondTree(std::uint32_t address) override {
        Check(address == kSystem + 28u,
              "second leading tree is cleaned before first");
        calls.push_back("second tree");
    }
    std::uint32_t CleanupFirstTree(std::uint32_t address) override {
        Check(address == kSystem + 12u,
              "first leading tree is cleaned last");
        calls.push_back("first tree");
        return 0xdeadu;
    }
};

struct CountRuntime final : pal4::IEffectSystemCountRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
};

struct EnableRuntime final : pal4::IEffectSystemEnableRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kManager = 0x2000u;
    static constexpr std::uint32_t kEffect = 0x3000u;
    bool active = true;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void CleanupEffects(std::uint32_t system) override {
        Check(system == kSystem, "disable cleans the same effect object");
        calls.push_back("cleanup effects");
    }
    std::uint32_t FirstListElement(std::uint32_t list) override {
        calls.push_back("first " + std::to_string(list - kSystem));
        return words.at(words.at(list + 4u));
    }
    void RemoveInlineEffects(std::uint32_t list,
                             std::uint32_t first,
                             std::uint32_t head) override {
        Check(list == kSystem + 212u &&
              head == words.at(list + 4u) &&
              first == words.at(head),
              "disable removes the inline effect list");
        calls.push_back("remove inline");
    }
    void RemoveListElements(std::uint32_t list,
                            std::uint32_t first,
                            std::uint32_t head) override {
        Check(head == words.at(list + 4u) &&
              first == words.at(head),
              "disable removes each list from first to header");
        calls.push_back("remove " + std::to_string(list - kSystem));
    }
    void RemoveEffect(std::uint32_t manager,
                      std::uint32_t system) override {
        Check(manager == kManager && system == kSystem,
              "disable removes effect from active manager");
        calls.push_back("remove manager");
    }
    std::uint32_t InitializeSystem() override {
        calls.push_back("initialize");
        return kManager;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        if (address == kSystem + 4u) {
            calls.push_back("disable check");
            const auto it = bytes.find(address);
            return it == bytes.end() ? 0u : it->second;
        }
        Check(address == kManager, "enable reads global effect status");
        return active ? 1u : 0u;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write enabled");
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address - kSystem));
        words[address] = value;
    }
    void InitializeGeometryData(std::uint32_t address) override {
        Check(address == kSystem + 52u,
              "RenderWare geometry data starts at +52");
        calls.push_back("geometry");
    }
    void AddEffect(std::uint32_t manager,
                   std::uint32_t system) override {
        Check(manager == kManager && system == kSystem,
              "effect is registered with initialized manager");
        calls.push_back("add");
    }
};

struct FrameRuntime final : pal4::IEffectSystemFrameRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    std::uint8_t enabled = 1u;
    float scale = 1.1f;
    std::vector<std::string> calls;
    std::vector<double> values;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u,
              "frame update reads enabled flag at +4");
        calls.push_back("enabled");
        return enabled;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == kSystem + 44u,
              "frame update reads scale at +44");
        calls.push_back("scale");
        return scale;
    }
    void UpdateProperties2(double seconds) override {
        calls.push_back("properties2");
        values.push_back(seconds);
    }
    void UpdateTimers2(std::uint32_t system,
                       double seconds) override {
        Check(system == kSystem, "timer stage receives effect system");
        calls.push_back("timers2");
        values.push_back(seconds);
    }
    void UpdateProperties12(std::uint32_t system,
                            double seconds) override {
        Check(system == kSystem, "properties12 receives effect system");
        calls.push_back("properties12");
        values.push_back(seconds);
    }
    void UpdateProperties10(std::uint32_t system,
                            double seconds) override {
        Check(system == kSystem, "properties10 receives effect system");
        calls.push_back("properties10");
        values.push_back(seconds);
    }
    void UpdateProperties11(std::uint32_t system,
                            double seconds) override {
        Check(system == kSystem, "properties11 receives effect system");
        calls.push_back("properties11");
        values.push_back(seconds);
    }
    int UpdateTimers(std::uint32_t system,
                     double seconds) override {
        Check(system == kSystem, "final timer stage receives effect system");
        calls.push_back("timers");
        values.push_back(seconds);
        return -7;
    }
};

struct DrawRuntime final : pal4::IEffectSystemDrawRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kContext = 0x2000u;
    std::uint8_t enabled = 1u;
    std::uint32_t state_calls = 0u;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u, "draw checks enabled flag");
        calls.push_back("enabled");
        return enabled;
    }
    int SetRenderState(std::uint32_t state,
                       std::uint32_t value) override {
        calls.push_back("state " + std::to_string(state) + " " +
                        std::to_string(value));
        return ++state_calls == 10u ? 0x1234 : 0;
    }
    std::uint32_t GetDeviceState28() override {
        calls.push_back("get device 28");
        return 0xabcdu;
    }
    void SetDeviceState28(std::uint32_t value) override {
        calls.push_back("device 28 " + std::to_string(value));
    }
    void DrawProperties13(std::uint32_t system,
                          std::uint32_t context) override {
        Check(system == kSystem && context == kContext,
              "properties13 receives draw context");
        calls.push_back("draw 13");
    }
    void DrawProperties15(std::uint32_t system,
                          std::uint32_t context) override {
        Check(system == kSystem && context == kContext,
              "properties15 receives draw context");
        calls.push_back("draw 15");
    }
    void DrawProperties(std::uint32_t system,
                        std::uint32_t context) override {
        Check(system == kSystem && context == kContext,
              "base properties receive draw context");
        calls.push_back("draw base");
    }
    void DrawProperties16(std::uint32_t system,
                          std::uint32_t context) override {
        Check(system == kSystem && context == kContext,
              "properties16 receives draw context");
        calls.push_back("draw 16");
    }
    void DrawProperties14(std::uint32_t system,
                          std::uint32_t context) override {
        Check(system == kSystem && context == kContext,
              "properties14 receives draw context");
        calls.push_back("draw 14");
    }
};

struct ClumpRuntime final : pal4::IEffectSystemClumpRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kClump = 0x2000u;
    std::uint8_t enabled = 1u;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u,
              "clump processing checks effect enabled flag");
        calls.push_back("enabled");
        return enabled;
    }
    std::uint8_t ProcessAtomics(std::uint32_t clump,
                                std::uint32_t geometry_data) override {
        Check(clump == kClump && geometry_data == kSystem + 52u,
              "clump atomics receives +52 geometry data");
        calls.push_back("atomics");
        return 0x51u;
    }
    std::uint8_t ProcessAtomicsWrapper(
        std::uint32_t clump) override {
        Check(clump == kClump,
              "clump atomics wrapper receives the input clump");
        calls.push_back("wrapper");
        return 0x72u;
    }
};

struct QueueRuntime final : pal4::IEffectSystemObjectQueueRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kObject = 0x2000u;
    bool active = true;
    bool fallback = false;
    bool camera = false;
    std::string type = "ClumpObject";
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t chosen_offset = 0u;

    QueueRuntime() {
        constexpr std::uint32_t offsets[] = {
            68u, 80u, 92u, 104u, 116u, 128u,
            140u, 152u, 164u, 176u, 188u, 200u};
        for (const auto offset : offsets)
            words[kSystem + offset + 4u] = 0x9000u + offset;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u,
              "queue dispatch checks effect enabled flag");
        calls.push_back("enabled");
        return active ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::string_view ObjectTypeName(std::uint32_t object) override {
        Check(object == kObject, "queue dispatch reads object type");
        calls.push_back("type");
        return type;
    }
    bool UseFallbackQueue(std::uint32_t object) override {
        Check(object == kObject, "first virtual queue flag receives object");
        calls.push_back("fallback flag");
        return fallback;
    }
    bool UseCameraQueue(std::uint32_t object) override {
        Check(object == kObject, "second virtual queue flag receives object");
        calls.push_back("camera flag");
        return camera;
    }
    void AddToList(std::uint32_t list,
                   std::uint32_t head,
                   std::uint32_t object) override {
        Check(object == kObject &&
              head == words.at(list + 4u),
              "queue insertion receives object and list head");
        chosen_offset = list - kSystem;
        calls.push_back("list");
    }
    void AddInlineEffect(std::uint32_t list,
                         std::uint32_t object,
                         std::uint8_t enabled) override {
        Check(list == kSystem + 212u && object == kObject && enabled == 1u,
              "EffectObject enters enabled inline queue");
        chosen_offset = 212u;
        calls.push_back("inline");
    }
};

struct RemoveRuntime final : pal4::IEffectSystemObjectRemoveRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kObject = 0x2000u;
    static constexpr std::uint32_t kHead = 0x9000u;
    bool active = true;
    bool fallback = false;
    bool camera = false;
    std::string type = "ClumpObject";
    std::uint32_t inline_result = 0u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;

    RemoveRuntime() {
        words[kSystem + 164u + 4u] = kHead;
        words[kSystem + 164u + 8u] = 0u;
        words[kHead] = kHead;
        words[kHead + 4u] = kHead;
        words[kSystem + 212u + 4u] = 0xa000u;
        inline_result = 0xa000u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u,
              "removal checks effect enabled flag");
        return active ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::string_view ObjectTypeName(std::uint32_t object) override {
        Check(object == kObject, "removal classifies the requested object");
        return type;
    }
    bool UseFallbackQueue(std::uint32_t object) override {
        Check(object == kObject, "removal checks fallback flag");
        return fallback;
    }
    bool UseCameraQueue(std::uint32_t object) override {
        Check(object == kObject, "removal checks camera flag");
        return camera;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t FindInlineEffect(std::uint32_t tree,
                                   std::uint32_t object) override {
        Check(tree == kSystem + 212u && object == kObject,
              "inline lookup uses effect tree and object identity");
        return inline_result;
    }
    void FreeNode(std::uint32_t node) override {
        freed.push_back(node);
    }
};

void TestInactiveAndEmpty() {
    Runtime inactive;
    inactive.active = 0u;
    Check(pal4::EffectSystem::CleanupInstances(
              Runtime::kSystem, inactive) == Runtime::kState &&
          inactive.calls == (std::vector<std::string>{"initialize"}),
          "inactive effect system returns initialization flag without cleanup");

    Runtime empty;
    Check(pal4::EffectSystem::CleanupInstances(
              Runtime::kSystem, empty) == Runtime::kHead &&
          empty.calls == (std::vector<std::string>{"initialize"}),
          "empty instance tree returns its header");
}

void TestInstanceCleanup() {
    Runtime runtime;
    constexpr std::uint32_t first = 0x4000u;
    constexpr std::uint32_t second = 0x4040u;
    runtime.words[Runtime::kHead] = first;
    runtime.words[first + 28u] = 0x5000u;
    runtime.words[second + 28u] = 0u;
    runtime.next[first] = second;
    runtime.next[second] = Runtime::kHead;
    Check(pal4::EffectSystem::CleanupInstances(
              Runtime::kSystem, runtime) == Runtime::kHead &&
          runtime.words.at(Runtime::kHead) == Runtime::kHead &&
          runtime.calls == (std::vector<std::string>{
              "initialize", "destroy 20480", "free 20480",
              "erase 16384", "erase 16448"}),
          "XML payload is destroyed and freed before each instance erase");
}

void TestLoadCachedInstance() {
    LoadRuntime inactive;
    inactive.active = false;
    Check(pal4::EffectSystem::LoadEffectFromFile(
              LoadRuntime::kSystem, "spark", inactive) == 0u &&
          inactive.calls == (std::vector<std::string>{"initialize"}),
          "inactive effect system does not load files or instances");

    LoadRuntime cached;
    cached.cached = true;
    Check(pal4::EffectSystem::LoadEffectFromFile(
              LoadRuntime::kSystem, "spark", cached) ==
              LoadRuntime::kInstance &&
          cached.calls == (std::vector<std::string>{
              "initialize", "find", "create instance", "load instance"}),
          "cache hit loads a new instance without opening a file");

    LoadRuntime rejected;
    rejected.cached = true;
    rejected.load_ok = false;
    Check(pal4::EffectSystem::LoadEffectFromFile(
              LoadRuntime::kSystem, "spark", rejected) == 0u &&
          rejected.calls == (std::vector<std::string>{
              "initialize", "find", "create instance", "load instance",
              "destroy instance"}),
          "failed cached instance is destroyed");
}

void TestLoadFromFile() {
    LoadRuntime missing;
    missing.missing_file = true;
    Check(pal4::EffectSystem::LoadEffectFromFile(
              LoadRuntime::kSystem, "spark", missing) == 0u &&
          missing.calls == (std::vector<std::string>{
              "initialize", "find", "begin scripts", "directory", "open",
              "end scripts"}),
          "missing effect file cleans up script scope");

    LoadRuntime invalid;
    invalid.parse_ok = false;
    Check(pal4::EffectSystem::LoadEffectFromFile(
              LoadRuntime::kSystem, "spark", invalid) == 0u &&
          invalid.calls == (std::vector<std::string>{
              "initialize", "find", "begin scripts", "directory", "open",
              "create xml", "create adapter", "parse", "close",
              "destroy xml", "free xml", "destroy adapter", "end scripts"}),
          "invalid effect XML closes file and releases document before scope");

    LoadRuntime loaded;
    Check(pal4::EffectSystem::LoadEffectFromFile(
              LoadRuntime::kSystem, "spark", loaded) ==
              LoadRuntime::kInstance &&
          loaded.calls == (std::vector<std::string>{
              "initialize", "find", "begin scripts", "directory", "open",
              "create xml", "create adapter", "parse", "insert", "close",
              "create instance", "load instance", "destroy adapter",
              "end scripts"}),
          "parsed XML enters cache before instance load and remains owned there");

    LoadRuntime rejected;
    rejected.load_ok = false;
    Check(pal4::EffectSystem::LoadEffectFromFile(
              LoadRuntime::kSystem, "spark", rejected) == 0u &&
          rejected.calls == (std::vector<std::string>{
              "initialize", "find", "begin scripts", "directory", "open",
              "create xml", "create adapter", "parse", "insert", "close",
              "create instance", "load instance", "destroy instance",
              "destroy adapter", "end scripts"}),
          "failed loaded instance is destroyed after cache insertion");
}

void TestInsertionFailureCleanup() {
    LoadRuntime runtime;
    runtime.throw_on_insert = true;
    bool threw = false;
    try {
        (void)pal4::EffectSystem::LoadEffectFromFile(
            LoadRuntime::kSystem, "spark", runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls == (std::vector<std::string>{
              "initialize", "find", "begin scripts", "directory", "open",
              "create xml", "create adapter", "parse", "insert", "close",
              "destroy xml", "free xml", "destroy adapter", "end scripts"}),
          "cache insertion failure releases unowned XML and all scopes");
}

void TestInstanceLoadExceptionCleanup() {
    for (const bool cached : {false, true}) {
        LoadRuntime runtime;runtime.cached=cached;runtime.throw_on_load=true;
        bool threw{};
        try {(void)pal4::EffectSystem::LoadEffectFromFile(LoadRuntime::kSystem,"spark",runtime);}
        catch(const std::runtime_error&) {threw=true;}
        const auto expected=cached?std::vector<std::string>{
            "initialize","find","create instance","load instance","destroy instance"}:
            std::vector<std::string>{"initialize","find","begin scripts","directory","open",
            "create xml","create adapter","parse","insert","close","create instance",
            "load instance","destroy instance","destroy adapter","end scripts"};
        Check(threw && runtime.calls==expected,
            "instance exception destroys partial Dialog, retains cached XML and restores cold-load scope");
    }
}

void TestTreeLowerBounds() {
    TreeRuntime tree;
    tree.texts[TreeRuntime::kKey] = "m";
    Check(pal4::EffectSystemTree::LowerBound(
              TreeRuntime::kTree, TreeRuntime::kKey, tree) ==
              TreeRuntime::kHead &&
          pal4::EffectSystemTree::LowerBoundByBytes(
              TreeRuntime::kTree, TreeRuntime::kKey, tree) ==
              TreeRuntime::kHead,
          "empty effect cache lower bounds return header");
    constexpr std::uint32_t a = 0x4100u;
    constexpr std::uint32_t m = 0x4200u;
    constexpr std::uint32_t z = 0x4300u;
    tree.words[TreeRuntime::kHead + 4u] = m;
    tree.words[m] = a;
    tree.words[m + 8u] = z;
    tree.words[a] = TreeRuntime::kNil;
    tree.words[a + 8u] = TreeRuntime::kNil;
    tree.words[z] = TreeRuntime::kNil;
    tree.words[z + 8u] = TreeRuntime::kNil;
    tree.texts[a + 12u] = "a";
    tree.texts[m + 12u] = "m";
    tree.texts[z + 12u] = "z";
    for (const auto& [key, expected] :
         {std::pair{"0", a}, std::pair{"a", a},
          std::pair{"b", m}, std::pair{"m", m},
          std::pair{"n", z}, std::pair{"z", z},
          std::pair{"zz", TreeRuntime::kHead}}) {
        tree.texts[TreeRuntime::kKey] = key;
        Check(pal4::EffectSystemTree::LowerBound(
                  TreeRuntime::kTree, TreeRuntime::kKey, tree) == expected &&
              pal4::EffectSystemTree::LowerBoundByBytes(
                  TreeRuntime::kTree, TreeRuntime::kKey, tree) == expected,
              "effect cache lower bounds handle exact and gap keys");
    }
}

void TestInitializeLayout() {
    InitRuntime runtime;
    Check(pal4::EffectSystem::kObjectSize == 260u &&
          pal4::EffectSystem::Initialize(
              InitRuntime::kSystem, runtime) == InitRuntime::kSystem &&
          runtime.bytes.at(InitRuntime::kSystem + 4u) == 0u &&
          runtime.words.at(InitRuntime::kSystem + 8u) == 0u &&
          runtime.words.at(InitRuntime::kSystem + 48u) == 0u &&
          runtime.bytes.at(InitRuntime::kSystem + 52u) == 0xa5u &&
          runtime.words.at(InitRuntime::kSystem + 56u) == 0u &&
          runtime.words.at(InitRuntime::kSystem + 60u) == 0u &&
          runtime.words.at(InitRuntime::kSystem + 64u) == 0u &&
          runtime.words.at(InitRuntime::kSystem) == 0x84b19cu,
          "effect-system constructor initializes scalar fields and vtable");
    Check(runtime.list_count == 12u,
          "effect-system constructor creates twelve list heads");
    for (std::uint32_t index = 0u; index < 12u; ++index) {
        const auto offset = 68u + index * 12u;
        Check(runtime.bytes.at(InitRuntime::kSystem + offset) == 0xa5u &&
              runtime.words.at(InitRuntime::kSystem + offset + 4u) ==
                  0x9000u + index * 0x20u &&
              runtime.words.at(InitRuntime::kSystem + offset + 8u) == 0u,
              "each effect-system list stores status, head and zero count");
    }
    Check(runtime.calls.front() == "first tree" &&
          runtime.calls[1u] == "second tree" &&
          runtime.calls[2u] == "list" &&
          runtime.calls[13u] == "list" &&
          runtime.calls[14u] == "inline state" &&
          runtime.calls[15u] == "fourth tree" &&
          runtime.calls[16u] == "fifth tree" &&
          runtime.calls[17u] == "vtable",
          "effect-system constructor orders embedded initialization and vtable");
}

void TestEmbeddedTreeCleanup() {
    using Kind = EmbeddedRuntime::Kind;
    EmbeddedRuntime first;
    constexpr std::uint32_t root = 0x4000u;
    first.words[EmbeddedRuntime::kTree + 12u] = 3u;
    first.words[EmbeddedRuntime::kHead] = root;
    first.words[EmbeddedRuntime::kHead + 4u] = root;
    first.words[EmbeddedRuntime::kHead + 8u] = root;
    Check(pal4::EffectSystemEmbeddedTrees::CleanupFirst(
              EmbeddedRuntime::kTree, first) == 1u &&
          first.words.at(EmbeddedRuntime::kHead + 4u) ==
              first.nils.at(Kind::First) &&
          first.words.at(EmbeddedRuntime::kTree + 4u) == 0u &&
          first.words.at(EmbeddedRuntime::kTree + 12u) == 0u &&
          first.references.at(Kind::First) == 1u &&
          first.nils.at(Kind::Second) == 0x93cdc0u &&
          first.calls == (std::vector<std::string>{
              "delete first 16384", "first element", "free 8192",
              "release first"}),
          "first embedded tree resets a populated branch and retains shared nil");

    EmbeddedRuntime second;
    second.references[Kind::Second] = 1u;
    second.words[EmbeddedRuntime::kTree + 12u] = 1u;
    second.words[EmbeddedRuntime::kHead + 4u] = root;
    Check(pal4::EffectSystemEmbeddedTrees::CleanupSecond(
              EmbeddedRuntime::kTree, second) == 0x93cdc1u &&
          second.nils.at(Kind::Second) == 0u &&
          second.nils.at(Kind::First) == 0x93cdb8u &&
          second.calls == (std::vector<std::string>{
              "delete second 16384", "first element", "free 8192",
              "release second", "clear second",
              "free " + std::to_string(0x93cdc0u)}),
          "last second-tree owner frees only its own nil");
}

void TestEmbeddedTreeFallback() {
    EmbeddedRuntime runtime;
    constexpr std::uint32_t first = 0x4000u;
    constexpr std::uint32_t second = 0x4040u;
    runtime.words[EmbeddedRuntime::kHead] = first;
    runtime.successors[first] = second;
    runtime.successors[second] = EmbeddedRuntime::kHead;
    Check(pal4::EffectSystemEmbeddedTrees::CleanupFirst(
              EmbeddedRuntime::kTree, runtime) == 1u &&
          runtime.calls == (std::vector<std::string>{
              "next first 16384", "remove first 16384",
              "next first 16448", "remove first 16448",
              "free 8192", "release first"}),
          "zero-count fallback captures each successor before removal");

    EmbeddedRuntime empty;
    Check(pal4::EffectSystemEmbeddedTrees::CleanupSecond(
              EmbeddedRuntime::kTree, empty) == 1u &&
          empty.calls == (std::vector<std::string>{
              "free 8192", "release second"}),
          "empty second tree skips branch and iterator cleanup");
}

void TestOtherEmbeddedTrees() {
    using Kind = EmbeddedRuntime::Kind;
    EmbeddedRuntime third;
    third.words[EmbeddedRuntime::kTree + 12u] = 1u;
    third.words[EmbeddedRuntime::kHead + 4u] = 0x4400u;
    Check(pal4::EffectSystemEmbeddedTrees::CleanupThird(
              EmbeddedRuntime::kTree, third) == 1u &&
          third.words.at(EmbeddedRuntime::kHead + 4u) == 0x93cdc8u &&
          third.nils.at(Kind::Fourth) == 0x93cdd0u &&
          third.calls.front() == "delete third 17408",
          "third tree uses its own subtree destructor and nil");

    EmbeddedRuntime fourth;
    fourth.references[Kind::Fourth] = 1u;
    fourth.words[EmbeddedRuntime::kTree + 12u] = 1u;
    fourth.words[EmbeddedRuntime::kHead + 4u] = 0x4500u;
    Check(pal4::EffectSystemEmbeddedTrees::CleanupFourth(
              EmbeddedRuntime::kTree, fourth) == 0x93cdd1u &&
          fourth.nils.at(Kind::Fourth) == 0u &&
          fourth.nils.at(Kind::Third) == 0x93cdc8u &&
          fourth.calls.front() == "delete fourth 17664",
          "fourth tree frees only its own nil on final release");

    EmbeddedRuntime fifth;
    fifth.references[Kind::Fifth] = 1u;
    fifth.words[EmbeddedRuntime::kTree + 12u] = 1u;
    fifth.words[EmbeddedRuntime::kHead + 4u] = 0x4600u;
    Check(pal4::EffectSystemEmbeddedTrees::CleanupFifth(
              EmbeddedRuntime::kTree, fifth) == 0x93cdd9u &&
          fifth.nils.at(Kind::Fifth) == 0u &&
          fifth.nils.at(Kind::Fourth) == 0x93cdd0u &&
          fifth.calls.front() == "delete fifth 17920",
          "fifth tree frees only its own nil on final release");
}

void TestDestroyOrder() {
    DestroyRuntime runtime;
    Check(pal4::EffectSystem::Destroy(
              DestroyRuntime::kSystem, runtime) == 0xdeadu,
          "effect-system destructor returns first-tree cleanup result");
    std::vector<std::string> expected{
        "vtable", "disable check", "tail 244", "tail 228", "tail 212"};
    constexpr std::uint32_t offsets[] = {
        200u, 188u, 176u, 164u, 152u, 140u, 128u, 116u};
    for (const auto offset : offsets) {
        if (offset <= 164u)
            expected.push_back("first " + std::to_string(offset));
        expected.push_back("remove " + std::to_string(offset));
        expected.push_back("free " +
                           std::to_string(0x9000u + offset));
        Check(runtime.words.at(DestroyRuntime::kSystem + offset + 4u) == 0u &&
              runtime.words.at(DestroyRuntime::kSystem + offset + 8u) == 0u,
              "destructor clears each list head and count");
    }
    for (const auto offset : {104u, 92u, 80u, 68u})
        expected.push_back("config " + std::to_string(offset));
    expected.push_back("range");
    expected.push_back("free " + std::to_string(0xb000u));
    expected.push_back("second tree");
    expected.push_back("first tree");
    Check(runtime.calls == expected &&
          runtime.words.at(DestroyRuntime::kSystem + 56u) == 0u &&
          runtime.words.at(DestroyRuntime::kSystem + 60u) == 0u &&
          runtime.words.at(DestroyRuntime::kSystem + 64u) == 0u,
          "destructor releases all embedded structures in native reverse order");
}

void TestTotalCount() {
    CountRuntime runtime;
    for (std::uint32_t index = 0u; index < 12u; ++index)
        runtime.words[CountRuntime::kSystem + 76u + index * 12u] = index + 1u;
    runtime.words[CountRuntime::kSystem + 224u] = 9u;
    Check(pal4::EffectSystem::TotalCount(
              CountRuntime::kSystem, runtime) == 87u &&
          runtime.reads.size() == 13u &&
          runtime.reads.front() == CountRuntime::kSystem + 76u &&
          runtime.reads.back() == CountRuntime::kSystem + 224u,
          "effect-system count sums exactly twelve lists and inline state");
    for (std::uint32_t index = 0u; index < 12u; ++index)
        Check(runtime.reads[index] ==
                  CountRuntime::kSystem + 76u + index * 12u,
              "effect-system count reads list fields in native order");
    runtime.words[CountRuntime::kSystem + 76u] = 0xffffffffu;
    for (std::uint32_t index = 1u; index < 12u; ++index)
        runtime.words[CountRuntime::kSystem + 76u + index * 12u] = 0u;
    runtime.words[CountRuntime::kSystem + 224u] = 2u;
    Check(pal4::EffectSystem::TotalCount(
              CountRuntime::kSystem, runtime) == 1u,
          "effect-system count retains 32-bit wraparound");
}

void TestEnable() {
    EnableRuntime empty;
    Check(pal4::EffectSystem::Enable(
              EnableRuntime::kSystem, 0u, 0xaaaau, empty) == 0u &&
          empty.calls == (std::vector<std::string>{"disable check"}),
          "null effect stops after reset without initializing manager");

    EnableRuntime inactive;
    inactive.active = false;
    Check(pal4::EffectSystem::Enable(
              EnableRuntime::kSystem, EnableRuntime::kEffect,
              0u, inactive) == 0u &&
          inactive.calls == (std::vector<std::string>{
              "disable check", "initialize"}),
          "inactive global manager prevents effect attachment");

    EnableRuntime active;
    Check(pal4::EffectSystem::Enable(
              EnableRuntime::kSystem, EnableRuntime::kEffect,
              0xbbbbu, active) == 1u &&
          active.bytes.at(EnableRuntime::kSystem + 4u) == 1u &&
          active.words.at(EnableRuntime::kSystem + 48u) ==
              EnableRuntime::kEffect &&
          active.words.at(EnableRuntime::kSystem + 44u) == 0x3f800000u &&
          active.calls == (std::vector<std::string>{
              "disable check", "initialize", "write enabled", "write 48",
              "write 44", "geometry", "initialize", "add"}),
          "enable writes state, geometry and manager registration in order");
}

void TestDisable() {
    EnableRuntime inactive;
    pal4::EffectSystem::Disable(EnableRuntime::kSystem, inactive);
    Check(inactive.calls == (std::vector<std::string>{"disable check"}),
          "already disabled effect skips cleanup and manager lookup");

    EnableRuntime active;
    active.bytes[EnableRuntime::kSystem + 4u] = 1u;
    active.words[EnableRuntime::kSystem + 48u] = EnableRuntime::kEffect;
    constexpr std::uint32_t offsets[] = {
        212u, 68u, 92u, 80u, 104u, 116u, 128u, 140u,
        152u, 164u, 176u, 188u, 200u};
    for (const auto offset : offsets) {
        const auto head = 0x9000u + offset;
        active.words[EnableRuntime::kSystem + offset + 4u] = head;
        active.words[head] = 0xa000u + offset;
    }
    pal4::EffectSystem::Disable(EnableRuntime::kSystem, active);
    std::vector<std::string> expected{
        "disable check", "cleanup effects", "geometry",
        "write enabled", "write 48", "first 212", "remove inline"};
    for (std::size_t index = 1u; index < 13u; ++index) {
        const auto offset = offsets[index];
        if (offset >= 152u)
            expected.push_back("first " + std::to_string(offset));
        expected.push_back("remove " + std::to_string(offset));
    }
    expected.push_back("initialize");
    expected.push_back("remove manager");
    Check(active.bytes.at(EnableRuntime::kSystem + 4u) == 0u &&
          active.words.at(EnableRuntime::kSystem + 48u) == 0u &&
          active.calls == expected,
          "disable clears enabled state, lists and manager in native order");
}

void TestUpdateFrame() {
    FrameRuntime inactive;
    inactive.enabled = 0u;
    Check(pal4::EffectSystem::UpdateFrame(
              FrameRuntime::kSystem, 0.1, inactive) == 0 &&
          inactive.calls == (std::vector<std::string>{"enabled"}),
          "disabled effect skips scale read and all update stages");

    FrameRuntime active;
    constexpr double elapsed = 0.123456789;
    const double full = static_cast<double>(active.scale) * elapsed;
    const double rounded = static_cast<double>(static_cast<float>(full));
    Check(full != rounded,
          "frame fixture distinguishes double and float-rounded values");
    Check(pal4::EffectSystem::UpdateFrame(
              FrameRuntime::kSystem, elapsed, active) == -7 &&
          active.calls == (std::vector<std::string>{
              "enabled", "scale", "properties2", "timers2",
              "properties12", "properties10", "properties11", "timers"}) &&
          active.values.size() == 6u && active.values.front() == full,
          "first effect update keeps full scaled precision and stage order");
    for (std::size_t i = 1u; i < active.values.size(); ++i)
        Check(active.values[i] == rounded,
              "later effect updates receive float-rounded frame time");
}

void TestDraw() {
    DrawRuntime inactive;
    inactive.enabled = 0u;
    Check(pal4::EffectSystem::Draw(
              DrawRuntime::kSystem, DrawRuntime::kContext, inactive) == 0u &&
          inactive.calls == (std::vector<std::string>{"enabled"}),
          "disabled effect issues no render calls");

    DrawRuntime active;
    Check(pal4::EffectSystem::Draw(
              DrawRuntime::kSystem, DrawRuntime::kContext, active) == 0x34u &&
          active.state_calls == 10u &&
          active.calls == (std::vector<std::string>{
              "enabled", "state 6 1", "state 8 1", "state 12 1",
              "state 10 5", "state 11 6", "get device 28", "device 28 0",
              "draw 13", "draw 15", "draw base", "draw 16", "draw 14",
              "device 28 43981", "state 6 1", "state 8 1", "state 12 1",
              "state 10 5", "state 11 6"}),
          "draw sets and restores device state around five ordered groups");
}

void TestClumpAtomics() {
    ClumpRuntime inactive;
    inactive.enabled = 0u;
    Check(pal4::EffectSystem::ProcessClumpAtomics(
              ClumpRuntime::kSystem, ClumpRuntime::kClump, inactive) == 0u &&
          pal4::EffectSystem::ProcessClumpAtomicsWrapper(
              ClumpRuntime::kSystem, ClumpRuntime::kClump, inactive) == 0u &&
          inactive.calls == (std::vector<std::string>{
              "enabled", "enabled"}),
          "disabled system does not call either RenderWare clump service");

    ClumpRuntime active;
    Check(pal4::EffectSystem::ProcessClumpAtomics(
              ClumpRuntime::kSystem, ClumpRuntime::kClump, active) == 0x51u &&
          pal4::EffectSystem::ProcessClumpAtomicsWrapper(
              ClumpRuntime::kSystem, ClumpRuntime::kClump, active) == 0x72u &&
          active.calls == (std::vector<std::string>{
              "enabled", "atomics", "enabled", "wrapper"}),
          "active clump entries forward native return values");
}

void TestQueueObject() {
    QueueRuntime inactive;
    inactive.active = false;
    Check(pal4::EffectSystem::QueueObject(
              QueueRuntime::kSystem, QueueRuntime::kObject, inactive) == 0u &&
          inactive.calls == (std::vector<std::string>{"enabled"}),
          "disabled effect does not classify or queue objects");
    QueueRuntime null_object;
    Check(pal4::EffectSystem::QueueObject(
              QueueRuntime::kSystem, 0u, null_object) == 0u &&
          null_object.calls == (std::vector<std::string>{"enabled"}),
          "null object does not enter any queue");

    constexpr std::pair<std::string_view, std::uint32_t> cases[] = {
        {"ClumpObject", 164u}, {"RtkPrtStdAtomicObject", 176u},
        {"Sound3DObject", 188u}, {"CylinderObject", 140u},
        {"StripObject", 104u}, {"BillBordObject", 116u},
        {"CameraObject", 68u}, {"SequnceAnimObject", 128u},
        {"DynamicStripObject", 80u}, {"ThunderObject", 92u},
        {"OtherObject", 200u}};
    for (const auto& [name, expected] : cases) {
        QueueRuntime runtime;
        runtime.type = name;
        Check(pal4::EffectSystem::QueueObject(
                  QueueRuntime::kSystem, QueueRuntime::kObject,
                  runtime) == 1u &&
              runtime.chosen_offset == expected &&
              runtime.calls == (std::vector<std::string>{
                  "enabled", "type", "fallback flag", "camera flag", "list"}),
              "type names select the IDA-mapped effect queue");
    }
    QueueRuntime inline_effect;
    inline_effect.type = "EffectObject";
    Check(pal4::EffectSystem::QueueObject(
              QueueRuntime::kSystem, QueueRuntime::kObject,
              inline_effect) == 1u &&
          inline_effect.chosen_offset == 212u &&
          inline_effect.calls == (std::vector<std::string>{
              "enabled", "type", "fallback flag", "camera flag", "inline"}),
          "EffectObject uses the enabled inline queue");

    QueueRuntime fallback;
    fallback.type = "EffectObject";
    fallback.fallback = true;
    Check(pal4::EffectSystem::QueueObject(
              QueueRuntime::kSystem, QueueRuntime::kObject,
              fallback) == 1u &&
          fallback.chosen_offset == 200u &&
          fallback.calls == (std::vector<std::string>{
              "enabled", "type", "fallback flag", "list"}),
          "first virtual flag overrides name and second flag");
    QueueRuntime camera;
    camera.type = "EffectObject";
    camera.camera = true;
    Check(pal4::EffectSystem::QueueObject(
              QueueRuntime::kSystem, QueueRuntime::kObject,
              camera) == 1u &&
          camera.chosen_offset == 152u,
          "second virtual flag overrides EffectObject inline queue");
}

void TestRemoveObject() {
    RemoveRuntime inactive;
    inactive.active = false;
    Check(!pal4::EffectSystem::RemoveObject(
              RemoveRuntime::kSystem, RemoveRuntime::kObject, inactive) &&
          inactive.freed.empty(),
          "disabled effect does not remove queued objects");
    RemoveRuntime null_object;
    Check(!pal4::EffectSystem::RemoveObject(
              RemoveRuntime::kSystem, 0u, null_object),
          "null object is not removed");
    RemoveRuntime empty;
    Check(!pal4::EffectSystem::RemoveObject(
              RemoveRuntime::kSystem, RemoveRuntime::kObject, empty) &&
          empty.freed.empty(),
          "empty list removal is a no-op");

    RemoveRuntime list;
    constexpr std::uint32_t first = 0x9100u;
    constexpr std::uint32_t second = 0x9200u;
    list.words[RemoveRuntime::kSystem + 164u + 8u] = 2u;
    list.words[RemoveRuntime::kHead] = first;
    list.words[RemoveRuntime::kHead + 4u] = second;
    list.words[first] = second;
    list.words[first + 4u] = RemoveRuntime::kHead;
    list.words[first + 8u] = 0x3333u;
    list.words[second] = RemoveRuntime::kHead;
    list.words[second + 4u] = first;
    list.words[second + 8u] = RemoveRuntime::kObject;
    Check(pal4::EffectSystem::RemoveObject(
              RemoveRuntime::kSystem, RemoveRuntime::kObject, list) &&
          list.words.at(first) == RemoveRuntime::kHead &&
          list.words.at(RemoveRuntime::kHead + 4u) == first &&
          list.words.at(RemoveRuntime::kSystem + 164u + 8u) == 1u &&
          list.freed == (std::vector<std::uint32_t>{second}),
          "list removal relinks neighbors, frees matching node and decrements count");

    RemoveRuntime inline_effect;
    inline_effect.type = "EffectObject";
    inline_effect.inline_result = 0xa100u;
    inline_effect.bytes[0xa100u + 16u] = 1u;
    Check(pal4::EffectSystem::RemoveObject(
              RemoveRuntime::kSystem, RemoveRuntime::kObject,
              inline_effect) &&
          inline_effect.bytes.at(0xa100u + 16u) == 0u &&
          inline_effect.freed.empty(),
          "inline EffectObject removal clears active byte without freeing node");
    inline_effect.inline_result = 0xa000u;
    Check(!pal4::EffectSystem::RemoveObject(
              RemoveRuntime::kSystem, RemoveRuntime::kObject,
              inline_effect),
          "missing inline EffectObject leaves tree unchanged");
}

}  // namespace

int main() {
    TestInactiveAndEmpty();
    TestInstanceCleanup();
    TestLoadCachedInstance();
    TestLoadFromFile();
    TestInstanceLoadExceptionCleanup();
    TestInsertionFailureCleanup();
    TestTreeLowerBounds();
    TestInitializeLayout();
    TestEmbeddedTreeCleanup();
    TestEmbeddedTreeFallback();
    TestOtherEmbeddedTrees();
    TestDestroyOrder();
    TestTotalCount();
    TestEnable();
    TestDisable();
    TestUpdateFrame();
    TestDraw();
    TestClumpAtomics();
    TestQueueObject();
    TestRemoveObject();
}
