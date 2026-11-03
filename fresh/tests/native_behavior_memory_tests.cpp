#include "pal4/native_event_document_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_string_buffer_runtime.h"
#include "pal4/native_effect_load_text.h"
#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_path_endpoint_save.h"
#include "pal4/binary_xml_config.h"
#include "pal4/script_event_data.h"

#include <array>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <map>
#include <stdexcept>
#include <string_view>
#include <string>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

// Faults only native document storage allocations, after construction.
class FailingMemory final : public pal4::IStringBufferStorageRuntime {
public:
    pal4::NativeBaseObjectMemory memory;
    unsigned calls = 0, fail_at = 0;
    unsigned temporary_record_allocations = 0;
    unsigned scalar_transfer_allocations = 0;
    unsigned fail_size_once = 0;
    unsigned raw_payload_allocations = 0;
    unsigned insertion_pair_allocations = 0, default_name_frees = 0;
    std::map<std::uint32_t, std::uint32_t> allocation_sizes;
    pal4::IStringBufferStorageRuntime* shared_records = nullptr;
    pal4::IStringBufferStorageRuntime& SharedRecordStorage() noexcept override {
        return shared_records ? *shared_records : *this;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        if (bytes == 28u) ++temporary_record_allocations;
        if (bytes == 1u || bytes == 4u) ++scalar_transfer_allocations;
        if (bytes == 37u) ++raw_payload_allocations;
        if (bytes == 44u) ++insertion_pair_allocations;
        if (fail_size_once != 0u && bytes == fail_size_once) {
            fail_size_once = 0u;
            throw std::bad_alloc{};
        }
        if (++calls == fail_at) throw std::bad_alloc{};
        const auto result = memory.Allocate(bytes);
        allocation_sizes[result] = bytes;
        return result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        const auto found = allocation_sizes.find(address);
        constexpr std::string_view unknown = "ATTR_NAME_UNKNOW";
        if (found != allocation_sizes.end()) {
            if (found->second >= unknown.size() + 2u &&
                std::string_view(reinterpret_cast<const char*>(address + 1u), unknown.size()) == unknown)
                ++default_name_frees;
            allocation_sizes.erase(found);
        }
        return memory.Free(address);
    }
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
    for(const std::string name : {std::string{},std::string{"H_169"},std::string(32u,'q'),std::string(63u,'z')}) {
        bool completed{};
        for(unsigned failure=1u;failure<12u;++failure) {
            FailingMemory storage;storage.fail_at=failure;
            bool failed{};
            try {
                pal4::NativeEffectLoadText text(storage,name);
                const auto key=text.NameRecordAddress();
                Check(text.Name()==name && storage.ReadDword(key+12u)==(name.empty()?0u:(name.size()|31u)),
                    "owned effect load name uses VC6 storage");
                Check(text.BuildPath("gamedata\\effect\\script\\")=="gamedata\\effect\\script\\"+name+".eft",
                    "effect path uses original directory/name/tail appends");
                text.ReleasePath();
                Check(text.Name()==name && storage.memory.OutstandingAllocations()==(name.empty()?0u:1u),
                    "path releases before the retained owned name");
                text.ReleasePath();
            } catch(const std::bad_alloc&){failed=true;}
            Check(storage.memory.OutstandingAllocations()==0u,"partial name/path failure leaves no allocations");
            if(!failed){completed=true;break;}
        }
        Check(completed,"bounded effect string failure sweep reaches successful load text");
    }
    {
        FailingMemory storage;
        pal4::NativeStringBufferRuntime strings(storage);
        pal4::StringBufferState text{};
        Check(strings.AllocateText(text,40u,1u),"native string initial grow");
        for(unsigned i=0;i<40u;++i)strings.WriteByte(text.data+i,'q');
        strings.SetTextSize(text,40u);
        auto borrowed=text;
        strings.WriteByte(text.data-1u,1u);
        Check(strings.AllocateText(text,80u,0u) && text.data!=borrowed.data &&
            text.length==40u && borrowed.length==40u &&
            strings.ReadByte(text.data+39u)=='q' && strings.ReadByte(borrowed.data-1u)==0u,
            "native shared string growth preserves payload and releases one reference");
        strings.SetErrorFlag(borrowed,1u);
        storage.fail_at=storage.calls+1u;
        bool failed{};
        try {(void)strings.AllocateText(text,5u,1u);}
        catch(const std::bad_alloc&){failed=true;}
        Check(failed && !text.data && !text.length && !text.capacity &&
            storage.memory.OutstandingAllocations()==0u,
            "failed large-buffer replacement publishes fully cleared state");
        storage.fail_at=0u;
        Check(strings.AllocateText(text,5u,1u) && text.capacity==31u && !text.length,
            "native string replacement retries after allocation failure");
        strings.SetErrorFlag(text,1u);
        Check(storage.memory.OutstandingAllocations()==0u,"native string retry releases all allocations");
    }
    {
        FailingMemory storage;
        pal4::NativeEventDocumentRuntime xml(storage);
        const auto document = xml.Allocate(60u);
        (void)xml.InitializeEmptyDocument(document);
        pal4::BinaryXmlNode root{"root", {
            {"c", 0u, std::int8_t{-128}}, {"i", 1u, std::int32_t{-42}},
            {"f", 2u, std::bit_cast<float>(0x80000000u)},
            {"e", 3u, std::string{}}}, {}};
        const auto bytes = pal4::BinaryXmlConfig::Serialize(root);
        Check(bytes.has_value(), "serialize binary XML scalar fixture");
        storage.temporary_record_allocations = storage.scalar_transfer_allocations = 0;
        Check(xml.LoadBinaryDocument(document, *bytes), "read typed binary XML through actual native adapter");
        Check(storage.temporary_record_allocations == 0u && storage.scalar_transfer_allocations == 0u,
            "binary XML stack attributes/scalars/empty string must not allocate transfer records (IDA5F77C0)");
        Check(xml.AttributeToInt(xml.FindNumericAttribute(document, "c"), 0) == -128 &&
              xml.AttributeToInt(xml.FindNumericAttribute(document, "i"), 0) == -42 &&
              std::bit_cast<std::uint32_t>(xml.AttributeToFloat(xml.FindNumericAttribute(document, "f"), 1.0f)) == 0x80000000u &&
              xml.AttributeToString(xml.FindNumericAttribute(document, "e")).empty(),
            "native binary XML preserves signed char/int, negative zero and empty string");
        storage.fail_size_once = 16u; // String-value owner allocation after its name is copied.
        bool failed = false;
        try { (void)xml.LoadBinaryDocument(document, *bytes); }
        catch (const std::bad_alloc&) { failed = true; }
        Check(failed && storage.fail_size_once == 0u && xml.DocumentCount() == 1u &&
                  xml.LoadBinaryDocument(document, *bytes) &&
                  xml.AttributeToInt(xml.FindNumericAttribute(document, "i"), 0) == -42,
            "partial stack attribute construction cleans its name and permits same-document retry");
        std::vector<std::uint8_t> raw_payload(37u, 8u);
        raw_payload[0] = 0u; raw_payload[1] = 255u;
        raw_payload[2] = 7u; raw_payload[3] = 4u;
        root.attributes.push_back({"b", 4u, raw_payload});
        root.children.push_back({"child", {{"s", 3u, std::string{"payload"}}}, {}});
        const auto recursive_bytes = pal4::BinaryXmlConfig::Serialize(root);
        storage.raw_payload_allocations = 0;
        Check(recursive_bytes && xml.LoadBinaryDocument(document, *recursive_bytes) &&
                  xml.AttributeToInt(xml.FindNumericAttribute(document, "b"), 0) == 0x0407ff00 &&
                  xml.ChildCount(document) == 1u &&
                  xml.AttributeToString(xml.FindNumericAttribute(xml.ChildAt(document, 0u), "s")) == "payload",
            "binary raw payload/size and recursive nonempty text retain independent owned storage");
        Check(storage.raw_payload_allocations == 1u,
            "only the binary input transfer buffer uses game heap; owned raw values use CRT malloc/free (IDA5F4FD0/5F4044)");
        xml.DestroyDocument(document); (void)xml.Free(document);
        Check(storage.memory.OutstandingAllocations() == 0u && xml.DocumentCount() == 0u,
            "binary XML reader releases owned values and borrowed attribute lifetime");
    }
    {
        FailingMemory storage;
        pal4::NativeEventDocumentRuntime xml(storage);
        const auto source = xml.Allocate(60u);
        (void)xml.InitializeDocument(source, "root", 0u);
        xml.WriteIntAttribute(source, "value", -17);
        const auto child = xml.CreateChild(source, "child");
        xml.WriteFloatAttribute(child, "value", 3.25f);
        storage.temporary_record_allocations = 0;
        const auto clone = xml.CloneDocument(source);
        Check(storage.temporary_record_allocations == 0u,
            "XML clone must keep its 28-byte attribute temporary off the game heap (IDA5F57E3)");
        const auto source_attribute = xml.FindNumericAttribute(source, "value");
        const auto cloned_attribute = xml.FindNumericAttribute(clone, "value");
        const auto source_name = storage.memory.ReadDword(source_attribute + 4u);
        Check(source_name == storage.memory.ReadDword(cloned_attribute + 4u),
            "XML attribute copy reuses existing VC6 whole-string sharing instead of deep-copying names (IDA5F3E18)");
        Check(xml.AttributeToInt(xml.FindNumericAttribute(clone, "value"), 0) == -17 &&
                  xml.DocumentCount() == 4u,
            "stack temporary retains recursive document and numeric attribute copies");
        xml.WriteIntAttribute(clone, "value", 9);
        Check(xml.AttributeToInt(xml.FindNumericAttribute(source, "value"), 0) == -17,
            "clone values remain independent after temporary destruction");
        xml.DestroyDocument(source); (void)xml.Free(source);
        Check(xml.AttributeToInt(xml.FindNumericAttribute(clone, "value"), 0) == 9 &&
                  std::string_view(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(source_name)), 5u) == "value",
            "shared attribute name survives original document destruction");
        xml.DestroyDocument(clone); (void)xml.Free(clone);
        Check(storage.memory.OutstandingAllocations() == 0u && xml.DocumentCount() == 0u,
            "recursive clone releases all owned values without freeing borrowed temporary storage");
    }
    {
        FailingMemory storage;
        pal4::NativeEventDocumentRuntime xml(storage);
        pal4::ScriptEventDataState event{};
        const auto address = reinterpret_cast<std::uint32_t>(&event);
        constexpr std::array<std::string_view, 1> names{"behAIAtomic-attribute"};
        xml.SetAtomicNodeNames(address, names);
        storage.temporary_record_allocations = 0;
        xml.AddAtomicIntAttribute(address, 0u, "behAIAtomic-classID", -17);
        xml.AddAtomicFloatAttribute(address, 0u, "beh-vtm-waitTime",
            std::bit_cast<float>(0x80000000u));
        Check(storage.temporary_record_allocations == 0u,
            "behavior atomic writers reuse original stack attribute lifetime (IDA42D230/431CE0)");
        const auto node = event.words[6];
        const auto integer = xml.FindNumericAttribute(node, "behAIAtomic-classID");
        const auto real = xml.FindNumericAttribute(node, "beh-vtm-waitTime");
        Check(xml.AttributeToInt(integer, 0) == -17 &&
                  std::bit_cast<std::uint32_t>(xml.AttributeToFloat(real, 1.0f)) == 0x80000000u,
            "atomic writers preserve signed integer and exact negative zero");
        const auto populated = storage.memory.OutstandingAllocations();
        xml.AddAtomicIntAttribute(address, 0u, "beh-vtm-waitTime", -7);
        Check(xml.FindNumericAttribute(node, "beh-vtm-waitTime") == real &&
                  xml.AttributeToFloat(real, 0.0f) == -7.0f &&
                  storage.memory.ReadDword(real + 16u) == 2u &&
                  storage.memory.OutstandingAllocations() == populated,
            "atomic overwrite retains node identity and original setter type");
        storage.fail_size_once = 60u; // Attribute tree node after key construction.
        bool failed = false;
        try { xml.AddAtomicFloatAttribute(address, 0u, "beh-vtm-expectTime", 2.5f); }
        catch (const std::bad_alloc&) { failed = true; }
        Check(failed && storage.memory.OutstandingAllocations() == populated &&
                  xml.FindNumericAttribute(node, "beh-vtm-expectTime") == 0u,
            "failed atomic insertion releases temporary name and preserves prior fields");
        xml.AddAtomicFloatAttribute(address, 0u, "beh-vtm-expectTime", 4.5f);
        Check(xml.AttributeToFloat(xml.FindNumericAttribute(node,
                  "beh-vtm-expectTime"), 0.0f) == 4.5f,
            "atomic insertion retries on the same initialized node");
        xml.CleanupEventData(address);
        Check(xml.DocumentCount() == 0u && storage.memory.OutstandingAllocations() == 0u,
            "atomic event releases owned nodes without freeing stack attributes");
    }
    // Exercise both array growth and spare-capacity append with the actual
    // native XML/string ownership. Every failed allocation must leave the
    // previously published event usable for retry and final destruction.
    for (unsigned existing = 0u; existing < 4u; ++existing) {
        for (unsigned fault = 1u; ; ++fault) {
            FailingMemory storage;
            pal4::NativeEventDocumentRuntime xml(storage);
            pal4::ScriptEventDataState event{};
            const auto address = reinterpret_cast<std::uint32_t>(&event);
            for (unsigned index = 0u; index < existing; ++index)
                (void)xml.AppendBehaviorAtomic(address, static_cast<std::int32_t>(index),
                    static_cast<std::int32_t>(10u + index));
            const auto before = event.words;
            const auto allocations = storage.memory.OutstandingAllocations();
            const auto documents_before = xml.DocumentCount();
            storage.calls = 0u;
            storage.fail_at = fault;
            bool failed = false;
            try { (void)xml.AppendBehaviorAtomic(address, -17, 37); }
            catch (const std::bad_alloc&) { failed = true; }
            storage.fail_at = 0u;
            if (failed) {
                Check(event.words == before && xml.DocumentCount() == documents_before &&
                          storage.memory.OutstandingAllocations() == allocations,
                    "failed atomic append preserves array, document count and allocation ownership");
                (void)xml.AppendBehaviorAtomic(address, -17, 37);
            }
            for (unsigned index = 0u; index < existing; ++index) {
                const auto node = event.words[6] + index * 60u;
                Check(xml.AttributeToInt(xml.FindNumericAttribute(node,
                          "behAIAtomic-classID"), -1) == static_cast<std::int32_t>(index) &&
                          xml.AttributeToInt(xml.FindNumericAttribute(node,
                          "behAIAtomic-proportion"), -1) == static_cast<std::int32_t>(10u + index),
                    "array growth and failed copies preserve earlier behavior values");
            }
            const auto last = event.words[7] - 60u;
            Check(xml.AttributeToInt(xml.FindNumericAttribute(last,
                      "behAIAtomic-classID"), 0) == -17 &&
                      storage.temporary_record_allocations == 0u,
                "successful atomic append copies the signed value without heap temporaries");
            xml.CleanupEventData(address);
            Check(xml.DocumentCount() == 0u && storage.memory.OutstandingAllocations() == 0u,
                "faulted/retried atomic array releases all copied names, attributes and documents");
            if (!failed) break;
            Check(fault < 128u, "atomic append allocation fault sweep is bounded");
        }
    }
    {
        FailingMemory storage;
        pal4::NativeEventDocumentRuntime xml(storage);
        const auto node = xml.Allocate(60u);
        (void)xml.InitializeDocument(node, "root", 0u);
        storage.insertion_pair_allocations = storage.default_name_frees = 0u;
        xml.WriteIntAttribute(node, "hp", -17);
        Check(storage.insertion_pair_allocations == 0u,
            "XML insertion key/attribute pair uses stack storage (IDA5F5C60)");
        Check(storage.default_name_frees == 1u &&
                  xml.AttributeToInt(xml.FindNumericAttribute(node, "hp"), 0) == -17,
            "XML insertion initializes then assigns over the actual default unknown attribute");
        xml.WriteIntAttribute(node, "hp", 9);
        Check(storage.default_name_frees == 1u &&
                  xml.AttributeToInt(xml.FindNumericAttribute(node, "hp"), 0) == 9,
            "existing XML attribute uses its setter without another insertion temporary");
        const pal4::BinaryXmlNode empty_key{"root",
            {{"", 3u, std::string{"empty-key-owned"}}}, {}};
        const auto bytes = pal4::BinaryXmlConfig::Serialize(empty_key);
        Check(bytes && xml.LoadBinaryDocument(node, *bytes) &&
                  xml.AttributeToString(xml.FindNumericAttribute(node, "")) == "empty-key-owned",
            "empty attribute name still owns its independent string value");
        const auto clone = xml.CloneDocument(node);
        Check(xml.AttributeToString(xml.FindNumericAttribute(clone, "")) == "empty-key-owned",
            "empty attribute name/value survives native document copying");
        xml.DestroyDocument(clone); (void)xml.Free(clone);
        xml.DestroyDocument(node); (void)xml.Free(node);
        Check(xml.DocumentCount() == 0u && storage.memory.OutstandingAllocations() == 0u,
            "XML insertion releases lookup strings, default values, pairs and stored attributes");
    }
    {
        pal4::NativeBaseObjectMemory survivor_storage;
        pal4::NativeEventDocumentRuntime survivor(survivor_storage);
        const auto surviving_node = survivor.Allocate(60u);
        std::uint32_t nil = 0u;
        {
            pal4::NativeBaseObjectMemory creator_storage;
            pal4::NativeEventDocumentRuntime creator(creator_storage);
            const auto first = creator.Allocate(60u);
            (void)creator.InitializeDocument(first, "first", 0u);
            nil = creator.SharedNil();
            (void)survivor.InitializeDocument(surviving_node, "second", 0u);
            Check(nil != 0u && survivor.SharedNil() == nil &&
                      creator.SharedDocumentCount() == 2u && survivor.SharedDocumentCount() == 2u,
                "distinct native owners share the original XML nil within the game allocation domain");
            creator.DestroyDocument(first); (void)creator.Free(first);
            Check(creator.DocumentCount() == 0u && survivor.DocumentCount() == 1u &&
                      survivor.SharedNil() == nil && survivor.SharedDocumentCount() == 1u &&
                      creator_storage.OutstandingAllocations() == 0u,
                "destroying the nil creator releases its own storage but preserves the survivor");
        }
        survivor.WriteIntAttribute(surviving_node, "hp", -17);
        Check(survivor.SharedNil() == nil &&
                  survivor.AttributeToInt(survivor.FindNumericAttribute(surviving_node, "hp"), 0) == -17,
            "shared XML nil outlives the creating facade and remains usable");
        survivor.DestroyDocument(surviving_node); (void)survivor.Free(surviving_node);
        Check(survivor.SharedNil() == 0u && survivor.DocumentCount() == 0u &&
                  survivor.SharedDocumentCount() == 0u && survivor_storage.OutstandingAllocations() == 0u &&
                  static_cast<pal4::NativeBaseObjectMemory&>(
                      survivor_storage.SharedRecordStorage()).OutstandingAllocations() == 0u,
            "last XML document releases the process allocation-domain nil");
    }
    {
        FailingMemory shared, first_storage, second_storage, isolated_storage;
        first_storage.shared_records = second_storage.shared_records = &shared;
        pal4::NativeEventDocumentRuntime first(first_storage), second(second_storage),
            isolated(isolated_storage);
        const auto a = first.Allocate(60u), b = second.Allocate(60u), c = isolated.Allocate(60u);
        (void)first.InitializeDocument(a, "first", 0u);
        const auto nil = first.SharedNil();
        (void)isolated.InitializeDocument(c, "isolated", 0u);
        Check(isolated.SharedNil() != nil && isolated.SharedDocumentCount() == 1u &&
                  first.SharedDocumentCount() == 1u,
            "independent storage domains do not merge XML sentinels or references");
        isolated.DestroyDocument(c); (void)isolated.Free(c);
        for (const bool fail_nil : {true, false}) {
            (fail_nil ? shared : second_storage).fail_size_once = 60u;
            bool failed = false;
            try { (void)second.InitializeDocument(b, "second", 0u); }
            catch (const std::bad_alloc&) { failed = true; }
            Check(failed && first.SharedNil() == nil && second.SharedNil() == nil &&
                      first.DocumentCount() == 1u && second.DocumentCount() == 0u &&
                      first.SharedDocumentCount() == 1u && shared.memory.OutstandingAllocations() == 1u &&
                      second_storage.memory.OutstandingAllocations() == 1u,
                "shared nil/head allocation failure rolls back references without releasing a survivor");
        }
        (void)second.InitializeDocument(b, "second", 0u);
        first.DestroyDocument(a); (void)first.Free(a);
        second.WriteIntAttribute(b, "hp", 9);
        Check(second.SharedNil() == nil && second.SharedDocumentCount() == 1u &&
                  second.AttributeToInt(second.FindNumericAttribute(b, "hp"), 0) == 9,
            "failed secondary document can retry and survive creator teardown");
        second.DestroyDocument(b); (void)second.Free(b);
        Check(first.SharedNil() == 0u && first.SharedDocumentCount() == 0u &&
                  shared.memory.OutstandingAllocations() == 0u &&
                  first_storage.memory.OutstandingAllocations() == 0u &&
                  second_storage.memory.OutstandingAllocations() == 0u &&
                  isolated_storage.memory.OutstandingAllocations() == 0u,
            "all allocation domains release their own final XML storage");
    }
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
    for (const bool string_value : {false, true}) {
        for (unsigned fault = 1; ; ++fault) {
            FailingMemory failing;
            pal4::NativeEventDocumentRuntime xml(failing);
            const auto address = xml.Allocate(60u);
            (void)xml.InitializeDocument(address, "behAIAtomicMgr-memory", 0u);
            xml.WriteIntAttribute(address, "retained", -17);
            const auto before_failure = failing.memory.OutstandingAllocations();
            failing.calls = 0; failing.fail_at = fault;
            bool failed = false;
            const auto write = [&](float value) {
                if (string_value)
                    xml.WriteStringAttribute(address, "AAB_MST_memory_speed", "owned-value");
                else
                    xml.WriteFloatAttribute(address, "AAB_MST_memory_speed", value);
            };
            try { write(2.5f); }
            catch (const std::bad_alloc&) { failed = true; }
            failing.fail_at = 0;
            if (failed) {
                if (failing.memory.OutstandingAllocations() != before_failure)
                    std::fprintf(stderr, "insertion fault %u, string=%u: live=%zu baseline=%zu\n",
                        fault, unsigned(string_value), failing.memory.OutstandingAllocations(), before_failure);
                Check(xml.FindNumericAttribute(address, "AAB_MST_memory_speed") == 0u &&
                    failing.memory.OutstandingAllocations() == before_failure,
                    "failed insertion rolls back its tree node and storage");
                write(4.5f);
                const auto attribute = xml.FindNumericAttribute(address, "AAB_MST_memory_speed");
                Check(string_value ? xml.AttributeToString(attribute) == "owned-value" :
                                         xml.AttributeToFloat(attribute, 0) == 4.5f,
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
    }
    std::puts("PASS: native behavior memory/C03 save and insertion failures");
    return EXIT_SUCCESS;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: unexpected exception: %s\n", error.what());
    return EXIT_FAILURE;
}
