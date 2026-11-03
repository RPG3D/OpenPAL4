#include "pal4/pal_floor_attribute.h"
#include "pal4/floor_color_tree.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using std::uint32_t;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PalFloorAttributeRuntimeStub {
    std::uint8_t ReadByte(uint32_t address) override {
        return bytes[address];
    }
    uint32_t ReadDword(uint32_t address) override {
        return static_cast<uint32_t>(ReadByte(address)) |
            (static_cast<uint32_t>(ReadByte(address + 1u)) << 8u) |
            (static_cast<uint32_t>(ReadByte(address + 2u)) << 16u) |
            (static_cast<uint32_t>(ReadByte(address + 3u)) << 24u);
    }
    void WriteByte(uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteDword(uint32_t address, uint32_t value) override {
        dword_writes.emplace_back(address, value);
        for (uint32_t index = 0; index < 4u; ++index)
            WriteByte(address + index,
                      static_cast<std::uint8_t>(value >> (8u * index)));
    }
    uint32_t EmptyTextAddress() override { return 0x9000u; }
    void SetErrorFlag(uint32_t address, std::uint8_t flag) override {
        WriteByte(address, flag);
    }
    bool AllocateText(uint32_t address, uint32_t size,
                      std::uint8_t mode) override {
        Check(mode == 1u, "text allocation mode");
        ++allocations;
        if (!allocation_succeeds) return false;
        next_text += 0x100u;
        WriteDword(address + 4u, next_text);
        WriteDword(address + 12u, size);
        return true;
    }
    void DetachAndReset(uint32_t address) override {
        ++detach_count;
        WriteDword(address + 4u, 0u);
        WriteDword(address + 8u, 0u);
        WriteDword(address + 12u, 0u);
    }
    void SetTextSize(uint32_t address, uint32_t size) override {
        WriteDword(address + 8u, size);
        WriteByte(ReadDword(address + 4u) + size, 0u);
    }
    void CopyBytes(uint32_t destination, uint32_t source,
                   uint32_t size) override {
        for (uint32_t index = 0; index < size; ++index)
            WriteByte(destination + index, ReadByte(source + index));
    }
    std::int32_t FloatToLong(long double value) override {
        converted.push_back(value);
        return static_cast<std::int32_t>(std::nearbyint(value));
    }
    std::int32_t CompareCaseInsensitive(
        uint32_t left, uint32_t right) override {
        comparisons.emplace_back(left, right);
        auto fold = [this](uint32_t address) {
            std::string value;
            while (ReadByte(address)) {
                auto ch = ReadByte(address++);
                if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
                value.push_back(static_cast<char>(ch));
            }
            return value;
        };
        return fold(left).compare(fold(right));
    }
    uint32_t GameDatabase() override { return 0x1000u; }
    uint32_t GetSingletonManager() override {
        events.emplace_back("manager");
        return 0xaaaau;
    }
    uint32_t Register(uint32_t manager, std::string_view name,
                      uint32_t id, uint32_t factory,
                      uint32_t destroy) override {
        Check(manager == 0xaaaau &&
                  name == "palFloorAttributeProcessor" && id == 1015u &&
                  factory == 0x41fda0u && destroy == 0x41fe00u,
              "singleton registration");
        events.emplace_back("register");
        return 0xbbbbu;
    }
    pal4::PalFloorAttributeState* Allocate(uint32_t bytes) override {
        Check(bytes == 60u, "object allocation size");
        events.emplace_back("allocate");
        return allocate_object ? &object : nullptr;
    }
    uint32_t Free(pal4::PalFloorAttributeState* state) override {
        Check(state == &object, "object free address");
        events.emplace_back("free");
        return 0x1234u;
    }
    uint32_t CreateEmptyList() override {
        events.emplace_back("list");
        WriteDword(0x7000u, 0x7000u);
        WriteDword(0x7004u, 0x7000u);
        return 0x7000u;
    }
    pal4::FloorColorTreeGlobals& ColorTreeGlobals() override {
        return globals;
    }
    uint32_t RgbaToPixel(uint32_t rgba, uint32_t format) override {
        Check(format == 1280u, "RenderWare pixel format");
        colors.push_back(rgba);
        return rgba ^ 0x13572468u;
    }
    uint32_t PrepareColorValue(std::string_view path,
                               uint32_t pixel) override {
        entries.emplace_back(std::string(path), pixel);
        if (throw_on_color_insert && entries.size() == 2u)
            throw std::runtime_error("injected color insertion failure");
        const auto index = static_cast<uint32_t>(entries.size());
        const auto value = 0x40000u + index * 0x100u;
        const auto data = 0x50000u + index * 0x100u;
        WriteDword(value, pixel);
        WriteByte(value + 4u, 0x5au);
        WriteDword(value + 8u, data);
        WriteDword(value + 12u, static_cast<uint32_t>(path.size()));
        WriteDword(value + 16u, static_cast<uint32_t>(path.size()));
        PutString(data, path);
        WriteByte(data - 1u, 0u);
        return value;
    }
    void ReleaseColorValue(uint32_t value) override {
        released_values.push_back(value);
    }
    void ReleaseText(pal4::FloorTextRecord&) override {
        events.emplace_back("release text");
    }
    void ReleaseList(pal4::PalFloorAttributeState&) override {
        events.emplace_back("release list");
    }
    uint32_t MovementAdapter() override { return movement_adapter; }
    uint32_t CallMovementVirtual(uint32_t object,
                                 uint32_t slot_bytes) override {
        Check(object == 0x12000u, "movement virtual receiver");
        virtual_calls.push_back(slot_bytes);
        return slot_bytes == 40u ? 0x13000u : triangle_index;
    }
    uint32_t AllocateListNode(uint32_t size) override {
        Check(size == 20u, "floor callback node size");
        next_node += 0x100u;
        return next_node;
    }
    uint32_t AllocateColorNode(uint32_t size) override {
        Check(size == 36u, "color tree node size");
        if (color_allocations++ == 0u) {
            events.emplace_back("tree");
            return 0x19200u;
        }
        if (color_allocations == 2u) return 0x19000u;
        next_allocated_color_node += 0x200u;
        return next_allocated_color_node;
    }
    void AssignSoundPath(pal4::FloorTextRecord&,
                         uint32_t source_record) override {
        assigned_sound_record = source_record;
        events.emplace_back("assign sound");
    }
    void AllocateEmptySoundPath(pal4::FloorTextRecord&) override {
        events.emplace_back("empty sound");
    }
    void InvokeCallback(uint32_t function, uint32_t context,
                        uint32_t pixel) override {
        callbacks.push_back({function, context, pixel});
    }
    uint32_t NextColorNode(uint32_t) override {
        events.emplace_back("next node");
        return next_color_node;
    }
    void DeleteColorNode(uint32_t node,
                         pal4::FloorColorTreeState&) override {
        deleted_color_nodes.push_back(node);
        events.emplace_back("delete node");
    }
    uint32_t FreeAddress(uint32_t address) override {
        freed_addresses.push_back(address);
        events.emplace_back("free address");
        return 0x1234u;
    }
    void SetTriangle(uint32_t first, uint32_t second, uint32_t third) {
        movement_adapter = 0x11000u;
        WriteDword(0x11000u + 8u, 0x12000u);
        WriteDword(0x13000u + 24u, 0x14000u);
        WriteDword(0x14000u + 48u, 0x15000u);
        WriteDword(0x14000u + 44u, 0x16000u);
        triangle_index = 1u;
        WriteDword(0x16000u + 8u, (2u << 16u) | 1u);
        WriteDword(0x16000u + 12u, 3u);
        WriteDword(0x15000u + 4u, first);
        WriteDword(0x15000u + 8u, second);
        WriteDword(0x15000u + 12u, third);
    }
    void PutString(uint32_t address, std::string_view value) {
        for (const char ch : value)
            WriteByte(address++, static_cast<std::uint8_t>(ch));
        WriteByte(address, 0u);
    }
    void SetRecord(uint32_t index, std::int32_t scaled,
                   uint32_t name, uint32_t path,
                   std::uint8_t count = 0u) {
        const auto record = 0x5000u + index * 40u;
        WriteDword(record + 8u, name);
        WriteDword(record + 20u, static_cast<uint32_t>(scaled));
        WriteDword(record + 28u, path);
        uint32_t size = 0u;
        while (ReadByte(path + size)) ++size;
        WriteDword(record + 32u, size);
        WriteDword(record + 36u, size);
        WriteByte(path - 1u, count);
    }
    std::map<uint32_t, std::uint8_t> bytes;
    std::vector<std::pair<uint32_t, uint32_t>> dword_writes;
    uint32_t next_text{0xa000u};
    bool allocation_succeeds{true};
    bool allocate_object{true};
    bool throw_on_color_insert{};
    unsigned color_allocations{};
    pal4::FloorColorTreeGlobals globals{};
    unsigned allocations{};
    unsigned detach_count{};
    uint32_t movement_adapter{};
    uint32_t triangle_index{};
    uint32_t assigned_sound_record{};
    uint32_t next_color_node{};
    uint32_t next_allocated_color_node{0x18f00u};
    uint32_t next_node{0x17000u};
    pal4::PalFloorAttributeState object{};
    std::vector<long double> converted;
    std::vector<std::pair<uint32_t, uint32_t>> comparisons;
    std::vector<std::string> events;
    std::vector<uint32_t> colors;
    std::vector<std::pair<std::string, uint32_t>> entries;
    std::vector<uint32_t> released_values;
    std::vector<uint32_t> virtual_calls;
    std::vector<std::array<uint32_t, 3>> callbacks;
    std::vector<uint32_t> deleted_color_nodes;
    std::vector<uint32_t> freed_addresses;
};

void SetDatabase(Runtime& runtime, uint32_t count) {
    runtime.WriteDword(0x1000u + 52u + 24u, 0x2000u);
    runtime.WriteDword(0x2000u + 96u, 0x5000u);
    runtime.WriteDword(0x2000u + 100u, 0x5000u + count * 40u);
    runtime.WriteDword(0x3000u + 4u, 0x4000u);
    runtime.PutString(0x4000u, "Floor");
}

void TestSoundRecordCount() {
    Runtime runtime;
    constexpr uint32_t collection = 0x2800u;
    runtime.WriteDword(collection + 8u, 0x5000u);
    Check(pal4::FloorSoundRecordCollection::Count(collection, runtime) == 0,
          "null collection begin has zero records");
    runtime.WriteDword(collection + 4u, 0x5000u);
    runtime.WriteDword(collection + 8u, 0x5000u + 3u * 40u);
    Check(pal4::FloorSoundRecordCollection::Count(collection, runtime) == 3,
          "collection count uses 40-byte records");
    runtime.WriteDword(collection + 8u, 0x5000u - 40u);
    Check(pal4::FloorSoundRecordCollection::Count(collection, runtime) == -1,
          "collection count uses signed division for reversed endpoints");
}

void TestLookupAndCopy() {
    Runtime runtime;
    SetDatabase(runtime, 3u);
    runtime.PutString(0x6000u, "other");
    runtime.PutString(0x6100u, "FLOOR");
    runtime.PutString(0x6200u, "floor");
    runtime.PutString(0x7000u, "first.wav");
    runtime.PutString(0x7100u, "second.wav");
    runtime.PutString(0x7200u, "third.wav");
    runtime.SetRecord(0u, 40, 0x6000u, 0x7000u);
    runtime.SetRecord(1u, 40, 0x6100u, 0x7100u);
    runtime.SetRecord(2u, 40, 0x6200u, 0x7200u);
    pal4::PalFloorAttribute::GetSoundPath(
        100, 150, 0x3000u, 0x8000u, runtime);
    Check(runtime.ReadDword(0x8000u + 4u) == 0x7100u,
          "first matching database record selected");
    Check(runtime.ReadDword(0x8000u + 8u) == 10u &&
              runtime.ReadByte(0x7100u - 1u) == 1u,
          "copy shares text and increments reference byte");
    Check(runtime.allocations == 0u, "shared copy has no allocation");
    Check(runtime.comparisons.front().first == 0x6000u,
          "case insensitive comparison uses candidate first");
    Check(runtime.converted.front() > 39.2L &&
              runtime.converted.front() < 39.3L,
          "x87 scale input");

    runtime.WriteByte(0x7100u - 1u, 0xfeu);
    pal4::PalFloorAttribute::GetSoundPath(
        100, 150, 0x3000u, 0x8100u, runtime);
    const auto copied = runtime.ReadDword(0x8100u + 4u);
    Check(copied != 0x7100u && runtime.allocations == 1u,
          "saturated reference byte triggers deep copy");
    Check(runtime.ReadByte(copied) == 's' &&
              runtime.ReadByte(copied + 10u) == 0u,
          "deep copy includes terminator");
    runtime.allocation_succeeds = false;
    pal4::PalFloorAttribute::GetSoundPath(
        100, 150, 0x3000u, 0x8200u, runtime);
    Check(runtime.ReadDword(0x8200u + 4u) == 0u &&
              runtime.ReadDword(0x8200u + 8u) == 0u,
          "failed text allocation leaves empty output");
}

void TestPriorityAndEmpty() {
    Runtime runtime;
    SetDatabase(runtime, 2u);
    runtime.PutString(0x6100u, "floor");
    runtime.PutString(0x7100u, "minus.wav");
    runtime.PutString(0x7200u, "previous.wav");
    runtime.SetRecord(0u, 39, 0x6100u, 0x7100u);
    runtime.SetRecord(1u, 59, 0x6100u, 0x7200u);
    pal4::PalFloorAttribute::GetSoundPath(
        100, 150, 0x3000u, 0x7f00u, runtime);
    Check(runtime.ReadDword(0x7f00u + 4u) == 0x7100u &&
              runtime.converted.size() == 1u,
          "current value wins without querying fallbacks");
    pal4::PalFloorAttribute::GetSoundPath(
        101, 150, 0x3000u, 0x8000u, runtime);
    Check(runtime.ReadDword(0x8000u + 4u) == 0x7100u,
          "current minus one precedes previous value");
    runtime.SetRecord(0u, 78, 0x6100u, 0x7100u);
    runtime.SetRecord(1u, 59, 0x6100u, 0x7200u);
    pal4::PalFloorAttribute::GetSoundPath(
        101, 150, 0x3000u, 0x8100u, runtime);
    Check(runtime.ReadDword(0x8100u + 4u) == 0x7200u,
          "previous value is fourth fallback");
    SetDatabase(runtime, 0u);
    pal4::PalFloorAttribute::GetSoundPath(
        101, 150, 0x3000u, 0x8200u, runtime);
    Check(runtime.allocations == 1u &&
              runtime.ReadDword(0x8200u + 8u) == 0u &&
              runtime.ReadByte(runtime.ReadDword(0x8200u + 4u)) == 0u,
          "missing record returns allocated empty text");
    runtime.WriteDword(0x2000u + 96u, 0u);
    runtime.WriteDword(0x2000u + 100u, 0u);
    Check(pal4::PalFloorAttribute::FindSoundRecord(
              0x3000u, 101, runtime) == 0u,
          "null collection begin has zero elements");
}

void TestMatcherAndAccess() {
    Runtime runtime;
    SetDatabase(runtime, 1u);
    runtime.PutString(0x6100u, "floor");
    runtime.PutString(0x7100u, "x");
    runtime.SetRecord(0u, 40, 0x6100u, 0x7100u);
    Check(!pal4::PalFloorAttribute::Matches(
              0x3000u, 39, 0x5000u, runtime) &&
              runtime.comparisons.empty(),
          "numeric mismatch avoids string comparison");
    Check(pal4::PalFloorAttribute::Matches(
              0x3000u, 40, 0x5000u, runtime),
          "case insensitive name equality");
    runtime.WriteDword(0x3000u + 4u, 0u);
    runtime.WriteDword(0x5000u + 8u, 0u);
    Check(pal4::PalFloorAttribute::Matches(
              0x3000u, 40, 0x5000u, runtime) &&
              runtime.comparisons.back() ==
                  std::pair<uint32_t, uint32_t>(0x9000u, 0x9000u),
          "null names use empty-text singleton");
    pal4::PalFloorAttributeAccessState access{};
    Check(pal4::PalFloorAttribute::GetSingleton(access, runtime) == 0xbbbbu &&
              pal4::PalFloorAttribute::GetSingleton(access, runtime) == 0xbbbbu &&
              runtime.events == std::vector<std::string>{"manager", "register"},
          "singleton registered once");
}

void TestConstructionAndDestruction() {
    Runtime runtime;
    auto* object = pal4::PalFloorAttribute::Create(runtime);
    Check(object == &runtime.object && object->list_head == 0x7000u &&
              object->list_size == 0u, "object initializes list");
    Check(runtime.entries.size() == 15u && runtime.colors.size() == 16u,
          "constructor inserts all colors and final default");
    Check(object->color_tree.size == 15u &&
              runtime.released_values.size() == 15u &&
              pal4::FloorColorTree::Find(
                  object->color_tree, runtime.entries.front().second,
                  runtime) != object->color_tree.header &&
              pal4::FloorColorTree::Find(
                  object->color_tree, runtime.entries.back().second,
                  runtime) != object->color_tree.header,
          "constructor builds a searchable tree and releases temporaries");
    const std::vector<std::pair<uint32_t, std::string>> expected{
        {0x0099ccffu, "WF011.wav"}, {0x00003399u, "WF013.wav"},
        {0x000099ffu, "WF015.wav"}, {0x00969696u, "WF017.wav"},
        {0x00cccc33u, "WF019.wav"}, {0x00ffffffu, "WF021.wav"},
        {0x0099ffffu, "WF023.wav"}, {0x00008080u, "WF037.wav"},
        {0x00ffcc99u, "WF039.wav"}, {0x00ccffccu, "WF041.wav"},
        {0x00cc99ffu, "WF053.wav"}, {0x00ff99ccu, "WF021.wav"},
        {0x00ff6633u, "WF039.wav"}, {0x00ff00ffu, "WF021.wav"},
        {0x000000ffu, "WF017.wav"},
    };
    Check(pal4::PalFloorAttribute::ColorSounds().size() == expected.size(),
          "fixed catalog size");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        Check(runtime.entries[index].first ==
                  "gamedata\\palSound\\" + expected[index].second &&
                  runtime.colors[index] == (expected[index].first | 0xff000000u) &&
                  runtime.entries[index].second ==
                      (runtime.colors[index] ^ 0x13572468u),
              "constructor preserves color/path insertion order");
    }
    Check(object->previous_color == 0u &&
              object->current_color == (0xff000000u ^ 0x13572468u),
          "default floor color");
    Check(pal4::PalFloorAttribute::Destroy(object, 0u, runtime) == 0x1234u,
          "destroy returns allocator result");
    Check(runtime.events.size() == 23u &&
              runtime.events[0] == "allocate" &&
              runtime.events[1] == "list" &&
              runtime.events[2] == "tree" &&
              runtime.events[3] == "release text" &&
              runtime.events[19] == "free address" &&
              runtime.events[20] == "free address" &&
              runtime.events[21] == "release list" &&
              runtime.events[22] == "free" &&
              runtime.freed_addresses.size() == 17u &&
              runtime.freed_addresses[runtime.freed_addresses.size() - 2u] ==
                  0x19000u &&
              runtime.freed_addresses.back() == 0x19200u &&
              runtime.globals.nil == 0u && runtime.globals.owners == 0u,
          "constructor tree nodes, header and nil are freed before list");
    Check(pal4::PalFloorAttribute::Destroy(nullptr, 77u, runtime) == 77u,
          "null destroy preserves undefined incoming register");
    Runtime failed;
    failed.allocate_object = false;
    Check(pal4::PalFloorAttribute::Create(failed) == nullptr &&
              failed.events == std::vector<std::string>{"allocate"},
          "allocation failure avoids construction");
    Runtime interrupted;
    interrupted.throw_on_color_insert = true;
    try {
        static_cast<void>(pal4::PalFloorAttribute::Create(interrupted));
        Check(false, "injected constructor failure must propagate");
    } catch (const std::runtime_error&) {
        Check(interrupted.freed_addresses ==
                  std::vector<uint32_t>{0x19100u, 0x19000u, 0x19200u},
              "constructor failure uses color-tree exception cleanup");
        Check(interrupted.events == std::vector<std::string>{
                  "allocate", "list", "tree", "release text",
                  "free address", "free address", "free address",
                  "release list", "free"},
              "constructor failure releases members before storage");
    }
}

void TestColorTreeDestruction() {
    Runtime runtime;
    pal4::FloorColorTree::Initialize(runtime.object, 0u, 0u, false, runtime);
    auto& tree = runtime.object.color_tree;
    for (uint32_t index = 0u; index < 2u; ++index) {
        const auto value = runtime.PrepareColorValue("sound.wav", index + 1u);
        static_cast<void>(pal4::FloorColorTree::Insert(tree, value, runtime));
        runtime.ReleaseColorValue(value);
    }
    pal4::PalFloorAttribute::DestroyColorTree(tree, runtime);
    Check(tree.header == 0u && tree.size == 0u &&
              runtime.freed_addresses.size() == 4u,
          "nonempty color tree releases both nodes, header and nil");
    Check(runtime.ReadDword(0x19000u) == 0x19000u &&
              runtime.ReadDword(0x19000u + 4u) == 0x19200u &&
              runtime.ReadDword(0x19000u + 8u) == 0x19000u,
          "tree header reset before free");
    Check(runtime.freed_addresses[2] == 0x19000u &&
              runtime.freed_addresses[3] == 0x19200u,
          "header and last shared nil freed after nodes");
    Runtime stray;
    pal4::FloorColorTree::Initialize(stray.object, 0u, 0u, false, stray);
    auto& tree_without_nodes = stray.object.color_tree;
    stray.WriteDword(tree_without_nodes.header, 0x19300u);
    stray.next_color_node = tree_without_nodes.header;
    pal4::PalFloorAttribute::DestroyColorTree(tree_without_nodes, stray);
    Check(stray.deleted_color_nodes == std::vector<uint32_t>{0x19300u} &&
              stray.freed_addresses ==
                  std::vector<uint32_t>{0x19000u, 0x19200u},
          "zero-size tree deletes stray nodes and releases last shared nil");
}

void TestColorUpdate() {
    Runtime runtime;
    runtime.object.list_head = runtime.CreateEmptyList();
    pal4::FloorColorTree::Initialize(runtime.object, 0u, 0u, false, runtime);
    runtime.object.current_color = 0x11111111u;
    Check(pal4::PalFloorAttribute::UpdateColorFromMovement(
              runtime.object, runtime) == 0u &&
              runtime.virtual_calls.empty() && runtime.callbacks.empty(),
          "missing movement adapter leaves state unchanged");

    const auto selected = 0xff000044u ^ 0x13572468u;
    const auto alternate = 0xff000099u ^ 0x13572468u;
    static_cast<void>(pal4::PalFloorAttribute::AddFloorAttribute(
        runtime.object, runtime.object.current_color, 1u, 11u, runtime));
    static_cast<void>(pal4::PalFloorAttribute::AddFloorAttribute(
        runtime.object, selected, 2u, 22u, runtime));
    static_cast<void>(pal4::PalFloorAttribute::AddFloorAttribute(
        runtime.object, alternate, 3u, 33u, runtime));
    runtime.SetTriangle(0x10000033u, 0x20000044u, 0x20000044u);
    const auto mixed_return = pal4::PalFloorAttribute::UpdateColorFromMovement(
        runtime.object, runtime);
    Check(runtime.virtual_calls == std::vector<uint32_t>{40u, 36u} &&
              runtime.callbacks.back()[2] == selected &&
              runtime.object.current_alpha == 0x20u,
          "second and third vertex majority updates color and alpha");
    Check(mixed_return == 0x00004410u,
          "second/third alpha branch preserves mixed stack-byte return");
    Check(runtime.colors.size() == 3u &&
              runtime.colors[0] == 0xff000033u &&
              runtime.colors[1] == 0xff000044u,
          "triangle color table uses packed 16-bit indices and forces alpha");
    runtime.object.fallback_alpha = 0x55u;
    runtime.SetTriangle(0x10000011u, 0x20000022u, 0x30000033u);
    const auto fallback_return =
        pal4::PalFloorAttribute::UpdateColorFromMovement(runtime.object,
                                                         runtime);
    Check(runtime.callbacks.back()[2] == 0x11111111u &&
              runtime.object.current_alpha == 0x55u &&
              fallback_return == 0x55u,
          "no majority preserves current color and uses fallback alpha");
    runtime.SetTriangle(0x77000099u, 0x77000099u, 0x22000022u);
    const auto matched_return =
        pal4::PalFloorAttribute::UpdateColorFromMovement(runtime.object,
                                                         runtime);
    Check(runtime.callbacks.back()[2] == alternate &&
              runtime.object.current_alpha == 0x77u &&
              matched_return == 0x77u,
          "first and second vertex majority wins");
}

void TestCallbackListAndColorApplication() {
    Runtime runtime;
    auto& state = runtime.object;
    state.list_head = runtime.CreateEmptyList();
    pal4::FloorColorTree::Initialize(state, 0u, 0u, false, runtime);
    state.current_color = 0x111u;
    Check(pal4::PalFloorAttribute::AddFloorAttribute(
              state, 0x111u, 0xa1u, 0xb1u, runtime) == 1u &&
              pal4::PalFloorAttribute::AddFloorAttribute(
                  state, 0x222u, 0xa2u, 0xb2u, runtime) == 2u,
          "callback list count");
    const auto first = 0x17100u;
    const auto second = 0x17200u;
    Check(runtime.ReadDword(state.list_head) == first &&
              runtime.ReadDword(first) == second &&
              runtime.ReadDword(second) == state.list_head &&
              runtime.ReadDword(state.list_head + 4u) == second &&
              runtime.ReadDword(second + 4u) == first,
          "20-byte nodes inserted at list tail");
    const auto value = runtime.PrepareColorValue("mapped.wav", 0x222u);
    const auto inserted = pal4::FloorColorTree::Insert(
        state.color_tree, value, runtime);
    runtime.ReleaseColorValue(value);
    pal4::PalFloorAttribute::ApplyColor(state, 0x222u, runtime);
    Check(inserted.inserted &&
              runtime.assigned_sound_record == inserted.node + 16u &&
              runtime.callbacks ==
                  std::vector<std::array<uint32_t, 3>>{{0xb2u, 0xa2u, 0x222u}},
          "mapped color loads sound and invokes matching callback");
    runtime.callbacks.clear();
    pal4::PalFloorAttribute::ApplyColor(state, 0x333u, runtime);
    Check(runtime.events.back() == "empty sound" &&
              runtime.callbacks ==
                  std::vector<std::array<uint32_t, 3>>{{0xb1u, 0xa1u, 0x111u}},
          "unmapped color invokes callbacks for current color");
}

void TestCallbackPayloadCopy() {
    Runtime runtime;
    const pal4::FloorCallbackPayload payload{0x11223344u, 0x55667788u,
                                              0x99aabbccu};
    Check(pal4::FloorCallbackNode::CopyPayload(0u, payload, runtime) == 0u &&
              runtime.dword_writes.empty(),
          "null callback payload destination is untouched");
    constexpr uint32_t destination = 0x30000u;
    Check(pal4::FloorCallbackNode::CopyPayload(destination, payload,
                                                runtime) == destination &&
              runtime.dword_writes ==
                  std::vector<std::pair<uint32_t, uint32_t>>{
                      {destination, payload.pixel},
                      {destination + 4u, 0u},
                      {destination + 8u, 0u},
                      {destination + 8u, payload.callback},
                      {destination + 4u, payload.context}} &&
              runtime.ReadDword(destination) == payload.pixel &&
              runtime.ReadDword(destination + 4u) == payload.context &&
              runtime.ReadDword(destination + 8u) == payload.callback,
          "callback payload copies fields in original write order");
}

void TestFloorTextCopy() {
    Runtime runtime;
    runtime.WriteByte(0x5000u, 0x7au);
    runtime.WriteDword(0x5004u, 0x6000u);
    runtime.WriteDword(0x5008u, 5u);
    runtime.WriteDword(0x500cu, 7u);
    runtime.PutString(0x6000u, "sound");
    runtime.WriteByte(0x5fffu, 0u);
    pal4::PalFloorAttribute::CopyData(0x7000u, 0x5000u, runtime);
    Check(runtime.ReadByte(0x7000u) == 0x7au &&
              runtime.ReadDword(0x7004u) == 0x6000u &&
              runtime.ReadDword(0x7008u) == 5u &&
              runtime.ReadDword(0x700cu) == 7u &&
              runtime.ReadByte(0x5fffu) == 1u &&
              runtime.allocations == 0u,
          "floor text share copies fields without changing destination flag");
    runtime.WriteByte(0x5fffu, 0xfeu);
    pal4::PalFloorAttribute::CopyData(0x7100u, 0x5000u, runtime);
    const auto deep = runtime.ReadDword(0x7104u);
    Check(deep != 0x6000u && runtime.ReadDword(0x7108u) == 5u &&
              runtime.ReadByte(deep) == 's' &&
              runtime.ReadByte(deep + 5u) == 0u,
          "saturated reference triggers a terminated deep copy");
    runtime.allocation_succeeds = false;
    pal4::PalFloorAttribute::CopyData(0x7200u, 0x5000u, runtime);
    Check(runtime.ReadDword(0x7204u) == 0u &&
              runtime.ReadDword(0x7208u) == 0u,
          "allocation failure retains initialized empty record");
    runtime.allocation_succeeds = true;
    pal4::PalFloorAttribute::CopyData(0x7300u, 0x7300u, runtime);
    Check(runtime.detach_count == 2u &&
              runtime.ReadDword(0x7304u) == 0u,
          "self-copy takes two reset calls after clearing its own size");
}

}  // namespace

int main() {
    TestSoundRecordCount();
    TestLookupAndCopy();
    TestPriorityAndEmpty();
    TestMatcherAndAccess();
    TestConstructionAndDestruction();
    TestColorTreeDestruction();
    TestColorUpdate();
    TestCallbackListAndColorApplication();
    TestCallbackPayloadCopy();
    TestFloorTextCopy();
}
