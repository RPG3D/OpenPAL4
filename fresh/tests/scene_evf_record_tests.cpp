#include "pal4/scene_evf_record.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISceneEvfRecordRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> external;
    void InitializeEmptyString(pal4::SceneEvfRecordState& state,
                               std::uint32_t offset) override {
        calls.push_back("string:" + std::to_string(offset));
        for (auto i = 0u; i < 3u; ++i)
            state.words[offset / 4u + i] = 0u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "EVF list head uses twelve bytes");
        calls.push_back("allocate");
        return 0x800u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        external[address] = value;
    }
};

struct CleanupRuntime final : pal4::ISceneEvfRecordCleanupRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> external;
    std::map<std::uint32_t, pal4::EffectPropertyState> properties;
    std::uint32_t world = 0xb00u;
    std::uint32_t ReadListWord(std::uint32_t address) override {
        if (address == 0xc00u || address == 0xe00u)
            calls.push_back("clear-list");
        return external.at(address);
    }
    void WriteListWord(std::uint32_t address,
                       std::uint32_t value) override {
        external[address] = value;
    }
    void FreeListNode(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void ClearBufferRange(std::uint32_t begin,
                          std::uint32_t end) override {
        calls.push_back("clear-range:" + std::to_string(begin) + ":" +
                        std::to_string(end));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return external.at(address);
    }
    pal4::EffectPropertyState& PropertyAt(
        std::uint32_t address) override {
        calls.push_back("property:" + std::to_string(address));
        return properties.at(address);
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void DestroyFrame(std::uint32_t frame,
                      std::uint32_t flag) override {
        Check(flag == 1u, "frame uses deleting destructor flag one");
        calls.push_back("frame:" + std::to_string(frame));
    }
    std::uint32_t ClumpWorld(std::uint32_t clump) override {
        calls.push_back("world:" + std::to_string(clump));
        return world;
    }
    void RemoveClump(std::uint32_t world_handle,
                     std::uint32_t clump) override {
        calls.push_back("remove-clump:" +
                        std::to_string(world_handle) + ":" +
                        std::to_string(clump));
    }
    void DestroyClump(std::uint32_t clump) override {
        calls.push_back("destroy-clump:" + std::to_string(clump));
    }
    void RemoveListElement(std::uint32_t head,
                           std::uint32_t node) override {
        calls.push_back("remove-node:" + std::to_string(head) + ":" +
                        std::to_string(node));
    }
    int CleanupString(pal4::SceneEvfRecordState&,
                      std::uint32_t offset) override {
        calls.push_back("string:" + std::to_string(offset));
        return offset == 8u ? -11 : 0;
    }
};

struct StringRuntime final : pal4::ISceneEvfRecordStringRuntime {
    std::uint32_t offset = 0u;
    std::string value;
    std::uint32_t SetString(pal4::SceneEvfRecordState&,
                            std::uint32_t field,
                            std::string_view input) override {
        offset = field;
        value = input;
        return 0x55u;
    }
};

struct TypeRuntime final : pal4::ISceneEvfRecordTypeRuntime {
    std::vector<std::string> calls;
    std::uint32_t factory = 0x500u, created = 0xabcu;
    void DestroyOld(std::uint32_t object,
                    std::uint32_t flag) override {
        Check(flag == 1u, "old type object uses deleting flag one");
        calls.push_back("destroy:" + std::to_string(object));
    }
    std::uint32_t FindFactory(std::uint32_t owner,
                              std::uint32_t type) override {
        calls.push_back("find:" + std::to_string(owner) + ":" +
                        std::to_string(type));
        return factory;
    }
    std::uint32_t Create(std::uint32_t selected) override {
        Check(selected == factory, "selected factory invoked");
        calls.push_back("create");
        return created;
    }
    void BindOwnerField(std::uint32_t object,
                        std::uint32_t field_address) override {
        calls.push_back("bind:" + std::to_string(object) + ":" +
                        std::to_string(field_address));
    }
};

std::uint8_t Byte(const pal4::SceneEvfRecordState& state,
                  std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

}  // namespace

int main() {
    pal4::SceneEvfRecordState state;
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    Check(&pal4::SceneEvfRecord::Initialize(
              state, 0x1000u, 0x12345678u, runtime) == &state,
          "EVF constructor returns its object");
    Check(runtime.calls == std::vector<std::string>{
              "string:8", "string:28", "string:40", "string:52",
              "string:104", "allocate", "write:2048", "write:2052"},
          "native string and list allocation order");
    Check(runtime.external[0x800u] == 0x800u &&
              runtime.external[0x804u] == 0x800u &&
              state.words[39u] == 0x800u && state.words[40u] == 0u,
          "list head self links and count");
    Check(state.words[1u] == 0xffffffffu &&
              state.words[18u] == 0x12345678u &&
              state.words[19u] == 1u &&
              Byte(state, 80u) == 0x78u &&
              Byte(state, 152u) == 0x78u,
          "owner and primary record flags");
    Check(state.words[33u] == 0x1088u &&
              Byte(state, 136u) == 0x9bu &&
              Byte(state, 137u) == 0x37u &&
              Byte(state, 138u) == 0xc8u &&
              Byte(state, 139u) == 0x95u &&
              Byte(state, 140u) == 0x37u &&
              Byte(state, 141u) == 0xc8u &&
              Byte(state, 142u) == 0x32u &&
              Byte(state, 143u) == 0x95u,
          "inline bytes and original 32-bit pointer");
    for (const auto offset : {20u, 24u, 64u, 68u, 84u, 88u, 92u,
                              96u, 100u, 116u, 120u, 124u, 128u,
                              144u, 148u, 164u, 168u})
        Check(state.words[offset / 4u] == 0u,
              "native zero-initialized fields");
    Check(state.words[0u] == 0xdeadbeefu &&
              state.words[42u] == 0u,
          "unrelated header preserved and final field zeroed");

    CleanupRuntime cleanup;
    state.words[16u] = 0x900u;
    state.words[17u] = 0xa00u;
    state.words[21u] = 0x300u;
    state.words[22u] = 0x3a0u;
    state.words[23u] = 0x3c0u;
    state.words[24u] = 0x400u;
    state.words[25u] = 2u;
    state.words[29u] = 0x500u;
    state.words[30u] = 4u;
    state.words[31u] = 0x600u;
    state.words[32u] = 5u;
    state.words[41u] = 0x700u;
    state.words[42u] = 6u;
    state.words[39u] = 0xc00u;
    state.words[40u] = 1u;
    cleanup.external[0x3fcu] = 2u;
    cleanup.external[0xc00u] = 0xd00u;
    cleanup.external[0xc04u] = 0xd00u;
    cleanup.external[0xd00u] = 0xc00u;
    cleanup.external[0xd04u] = 0xc00u;
    cleanup.properties[0x400u].words[16u] = 1u;
    cleanup.properties[0x450u].words[17u] = 2u;
    Check(pal4::SceneEvfRecord::Cleanup(state, cleanup) == -11,
          "last string cleanup result forwarded");
    Check(cleanup.calls == std::vector<std::string>{
              "clear-list", "free:3328", "clear-range:768:928",
              "property:1104", "property:1024", "free:1020",
              "free:1280", "free:1536", "free:1792",
              "frame:2304", "world:2560", "remove-clump:2816:2560",
              "destroy-clump:2560",
              "free:3072", "string:104", "free:768",
              "string:52", "string:40", "string:28", "string:8"},
          "EVF cleanup follows native resource and string order");
    Check(cleanup.properties[0x400u].words[16u] == 0u &&
              cleanup.properties[0x450u].words[17u] == 0u &&
              cleanup.external[0xc00u] == 0xc00u &&
              cleanup.external[0xc04u] == 0xc00u &&
              state.words[16u] == 0u && state.words[17u] == 0u &&
              state.words[18u] == 0u &&
              state.words[21u] == 0u && state.words[22u] == 0u &&
              state.words[23u] == 0u && state.words[24u] == 0u &&
              state.words[25u] == 0u && state.words[29u] == 0u &&
              state.words[30u] == 0u && state.words[31u] == 0u &&
              state.words[32u] == 0u && state.words[39u] == 0u &&
              state.words[40u] == 0u && state.words[41u] == 0u &&
              state.words[42u] == 0u,
          "EVF cleanup clears all owned handles and lengths");

    pal4::SceneEvfRecordState empty{};
    empty.words[39u] = 0xe00u;
    CleanupRuntime empty_runtime;
    empty_runtime.external[0xe00u] = 0xe00u;
    empty_runtime.world = 0u;
    Check(pal4::SceneEvfRecord::Cleanup(
              empty, empty_runtime) == -11 &&
              empty_runtime.calls == std::vector<std::string>{
                  "clear-list", "clear-range:0:0", "free:3584",
                  "string:104", "free:0", "string:52", "string:40",
                  "string:28", "string:8"},
          "empty EVF cleanup skips optional resources and frees null range");

    StringRuntime string_runtime;
    char name[]{'a', 'b', 'c', '\0', 'd', '\0'};
    Check(pal4::SceneEvfRecord::SetName(
              empty, name, string_runtime) == 0x55u &&
              string_runtime.offset == 104u &&
              string_runtime.value == "abc",
          "EVF name setter forwards C-string length and return");

    TypeRuntime type_runtime;
    empty.words[18u] = 0x600u;
    empty.words[16u] = 0x900u;
    const std::uint8_t colors[6u][4u]{
        {0xc8u, 20u, 20u, 0x95u},
        {0x9fu, 0x9fu, 20u, 0x95u},
        {50u, 0xdeu, 0xe2u, 0x95u},
        {0xb3u, 85u, 0xbfu, 0x95u},
        {70u, 63u, 0xd6u, 0x95u},
        {0xd9u, 108u, 0u, 0x95u}};
    for (std::uint32_t type = 0u; type < 6u; ++type) {
        type_runtime.calls.clear();
        const auto result = pal4::SceneEvfRecord::SetTypeAndColor(
            empty, 0x1000u, type, type_runtime);
        Check(result == (type == 0u ? 20u : type == 1u ? 0x9fu : 0xbcu) &&
                  empty.words[1u] == type && empty.words[16u] == 0xabcu,
              "EVF type selects native return and created object");
        for (std::uint32_t index = 0u; index < 4u; ++index)
            Check(Byte(empty, 136u + index) == colors[type][index],
                  "EVF type writes exact inline color bytes");
        Check(type_runtime.calls == std::vector<std::string>{
                  "destroy:" + std::to_string(type == 0u ? 0x900u : 0xabcu),
                  "find:1536:" + std::to_string(type),
                  "create", "bind:2748:4100"},
              "old object destroyed before factory lookup and owner binding");
    }
    type_runtime.calls.clear();
    Check(pal4::SceneEvfRecord::SetTypeAndColor(
              empty, 0x1000u, 7u, type_runtime) == 0xbcu &&
              empty.words[1u] == 7u &&
              Byte(empty, 136u) == colors[5u][0u] &&
              type_runtime.calls.size() == 4u,
          "unknown type keeps inline color and still replaces object");
    type_runtime.calls.clear();
    type_runtime.factory = 0u;
    Check(pal4::SceneEvfRecord::SetTypeAndColor(
              empty, 0x1000u, 2u, type_runtime) == 0u &&
              empty.words[16u] == 0u &&
              type_runtime.calls == std::vector<std::string>{
                  "destroy:2748", "find:1536:2"},
          "missing factory leaves type object null");
    type_runtime.factory = 0x500u;
    type_runtime.created = 0u;
    type_runtime.calls.clear();
    Check(pal4::SceneEvfRecord::SetTypeAndColor(
              empty, 0x1000u, 3u, type_runtime) == 0u &&
              type_runtime.calls == std::vector<std::string>{
                  "find:1536:3", "create"},
          "failed creation does not bind owner field");

    CleanupRuntime strings_runtime;
    Check(pal4::SceneEvfRecord::CleanupEmbeddedStrings(
              empty, strings_runtime) == -11 &&
              strings_runtime.calls == std::vector<std::string>{
                  "string:52", "string:40", "string:28", "string:8"},
          "embedded strings clear in native reverse order");
}
