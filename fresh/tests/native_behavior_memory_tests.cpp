#include "pal4/native_event_document_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_path_endpoint_save.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string_view>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

// Faults only native document storage allocations, after construction.
class FailingMemory final : public pal4::IStringBufferStorageRuntime {
public:
    pal4::NativeBaseObjectMemory memory;
    unsigned calls = 0, fail_at = 0;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        if (++calls == fail_at) throw std::bad_alloc{};
        return memory.Allocate(bytes);
    }
    std::uint32_t Free(std::uint32_t address) override { return memory.Free(address); }
    std::uint32_t ReadDword(std::uint32_t address) override { return memory.ReadDword(address); }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        memory.WriteDword(address, value);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        memory.WriteByte(address, value);
    }
};

// Exercise the existing C03 save algorithm against its real XML boundary.
// Animation and grid services remain explicitly controlled test dependencies.
class SaveRuntime final : public pal4::IMonsterPathEndpointSaveRuntime {
public:
    SaveRuntime(pal4::NativeEventDocumentRuntime& documents,
        pal4::BehaviorPathEndpointState& helper) : documents(documents), helper(helper) {}
    pal4::NativeEventDocumentRuntime& documents;
    pal4::BehaviorPathEndpointState& helper;
    int stage = 0, cycle = 0;
    std::int32_t ScriptVersion() override { return 17 + cycle; }
    void WriteMemoryInt(std::uint32_t memory, std::string_view key, std::int32_t value) override {
        documents.WriteIntAttribute(memory, key, value);
    }
    void WriteMemoryFloat(std::uint32_t memory, std::string_view key, float value) override {
        documents.WriteFloatAttribute(memory, key, value);
    }
    float HelperSpeed(std::uint32_t address) override {
        Check(address == 0x3000u, "C03 save preserves helper identity");
        return 3.25f + cycle;
    }
    float AnimationTime(std::uint32_t address) override {
        Check(address == 0x1170u, "C03 uses target+0x14 owner animation");
        return 0.125f + cycle;
    }
    void RemoveGridElement(std::uint32_t grid, std::uint32_t element) override {
        Check(grid == 0x4000u && element == 0x3000u && stage++ == 0 &&
            helper.words[0xb8u / 4u] == 0x12345600u,
            "C03 clears only active byte before grid release");
    }
    std::uint32_t SelectAction(std::uint32_t animation, std::string_view name, float blend) override {
        Check(animation == 0x1170u && blend == 0.5f, "C03 idle animation blend");
        Check(name == (stage == 1 ? "C01" : "Z01"), "C03 idle fallback order");
        ++stage;
        return name == "Z01" ? 1u : 0u;
    }
    std::uint32_t SetAnimationFlag(std::uint32_t animation, std::uint32_t flag) override {
        Check(animation == 0x1170u && flag == 1 && stage++ == 3, "C03 restart follows fallback");
        return 19u;
    }
};
}

int main() try {
    FailingMemory memory;
    pal4::NativeEventDocumentRuntime documents(memory);
    const auto node = documents.Allocate(60u);
    (void)documents.InitializeDocument(node, "behAIAtomicMgr-memory", 0u);
    const auto baseline = memory.memory.OutstandingAllocations();
    Check(documents.AttributeToInt(documents.FindNumericAttribute(node, "absent"), -3) == -3 &&
        documents.AttributeToFloat(0u, -0.5f) == -0.5f, "absent memory attributes use fallback");

    pal4::MonsterBehaviorManagerState manager{};
    pal4::MonsterPathEndpointTargetState target{};
    pal4::BehaviorPathEndpointState helper{};
    manager.words[0x204u / 4u] = 0x4000u;
    manager.words[0x668u / 4u] = node;
    target.words[0x14u / 4u] = 0x1000u;
    SaveRuntime save(documents, helper);
    std::array<std::uint32_t, 5> original{};
    constexpr std::array<std::string_view, 5> keys{
        "AAB_MST_memory_last_behavior_id", "AAB_MST_memory_speed",
        "AAB_MST_memory_actiontime", "AABPedestrian_MST_memory_pathDirection",
        "AABPedestrian_MST_memory_wander"};
    std::size_t populated = 0;
    for (int cycle = 0; cycle < 3; ++cycle) {
        save.stage = 0; save.cycle = cycle;
        helper.words[0xb8u / 4u] = 0x12345601u;
        helper.words[0xc0u / 4u] = static_cast<std::uint32_t>(cycle == 1 ? -1 : 1);
        helper.words[0xc4u / 4u] = 0xff008000u | (cycle == 2 ? 1u : 0u);
        Check(pal4::MonsterPathEndpointC03Save::Release(target, manager, helper,
            0x3000u, save) == 19u && save.stage == 4, "shared C03 save completes");
        for (unsigned i = 0; i < keys.size(); ++i) {
            const auto attribute = documents.FindNumericAttribute(node, keys[i]);
            Check(attribute != 0u, "C03 writes all five memory fields");
            if (!cycle) original[i] = attribute;
            else Check(attribute == original[i], "repeated save retains existing tree nodes");
        }
        Check(documents.AttributeToInt(original[0], 0) == 17 + cycle &&
            documents.AttributeToFloat(original[1], 0) == 3.25f + cycle &&
            documents.AttributeToFloat(original[2], 0) == 0.125f + cycle &&
            documents.AttributeToInt(original[3], 0) == (cycle == 1 ? -1 : 1) &&
            documents.AttributeToInt(original[4], 0) == (cycle == 2 ? 1 : 0),
            "real memory tree reads saved state including signed direction and low-byte wander");
        if (!cycle) populated = memory.memory.OutstandingAllocations();
        else Check(memory.memory.OutstandingAllocations() == populated,
            "overwrite allocates no persistent duplicate attributes");
    }
    // Existing attribute types are retained by original setters.
    documents.WriteIntAttribute(node, keys[1], -7);
    Check(documents.AttributeToFloat(original[1], 0) == -7.0f &&
        memory.memory.ReadDword(original[1] + 16u) == 2u,
        "integer write to an existing float preserves its type");
    const auto before_invalid = memory.memory.OutstandingAllocations();
    for (const auto name : {std::string_view{}, std::string_view("bad\0key", 7)}) {
        bool rejected = false;
        try { documents.WriteIntAttribute(node, name, 1); }
        catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected, "invalid attribute name rejected before mutation");
    }
    Check(memory.memory.OutstandingAllocations() == before_invalid,
        "invalid names do not allocate");
    documents.CleanupDocument(node);
    Check(memory.memory.OutstandingAllocations() == baseline,
        "memory cleanup releases all five attribute trees");
    documents.DestroyDocument(node);
    (void)documents.Free(node);
    Check(memory.memory.OutstandingAllocations() == 0u && documents.DocumentCount() == 0u,
        "memory document releases shared nil and all storage");

    // Find every actual allocation in insertion, then fault each in turn.
    for (unsigned fault = 1; ; ++fault) {
        FailingMemory failing;
        pal4::NativeEventDocumentRuntime xml(failing);
        const auto address = xml.Allocate(60u);
        (void)xml.InitializeDocument(address, "behAIAtomicMgr-memory", 0u);
        xml.WriteIntAttribute(address, "retained", -17);
        const auto before_failure = failing.memory.OutstandingAllocations();
        failing.calls = 0; failing.fail_at = fault;
        bool failed = false;
        try { xml.WriteFloatAttribute(address, "AAB_MST_memory_speed", 2.5f); }
        catch (const std::bad_alloc&) { failed = true; }
        failing.fail_at = 0;
        if (failed) {
            Check(xml.FindNumericAttribute(address, "AAB_MST_memory_speed") == 0u &&
                failing.memory.OutstandingAllocations() == before_failure,
                "failed insertion rolls back its tree node and storage");
            xml.WriteFloatAttribute(address, "AAB_MST_memory_speed", 4.5f);
            Check(xml.AttributeToFloat(xml.FindNumericAttribute(address,
                "AAB_MST_memory_speed"), 0) == 4.5f,
                "failed insertion can retry on the same live document");
        }
        Check(xml.AttributeToInt(xml.FindNumericAttribute(address, "retained"), 0) == -17,
            "insertion failure/retry preserves other attributes");
        xml.CleanupDocument(address);
        xml.DestroyDocument(address);
        (void)xml.Free(address);
        Check(failing.memory.OutstandingAllocations() == 0u,
            "failed memory insertion leaves no native allocation");
        if (!failed) break;
        Check(fault < 32, "fault sweep is bounded");
    }
    std::puts("PASS: native behavior memory/C03 save and insertion failures");
    return EXIT_SUCCESS;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: unexpected exception: %s\n", error.what());
    return EXIT_FAILURE;
}
