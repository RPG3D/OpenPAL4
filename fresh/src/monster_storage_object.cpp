#include "pal4/monster_storage_object.h"

#include <stdexcept>

namespace pal4 {
namespace {

class StringRuntimeStub final : public IStringAssignRuntime {
public:
    void ThrowRange() override { throw std::out_of_range("string range"); }
    void ThrowLength() override { throw std::length_error("string length"); }
    void DetachAndReset(StringBufferState& state) override { state = {}; }
    bool AllocateText(StringBufferState&, std::uint32_t,
                      std::uint8_t) override { return false; }
    void SetTextSize(StringBufferState& state,
                     std::uint32_t length) override { state.length = length; }
    void SetErrorFlag(StringBufferState& state,
                      std::uint8_t) override { state = {}; }
    void Reallocate(StringBufferState& state,
                    std::uint32_t length) override {
        state.data = 1u;
        state.capacity = length;
    }
    std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void Copy(std::uint32_t, std::uint32_t,
              std::uint32_t) override {}
    void Free(std::uint32_t) override {}
};

}  // namespace

std::uint32_t MonsterStorageObject::GetTypeCode() { return 11u; }

std::uint32_t MonsterStorageRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
std::uint32_t MonsterStorageRuntimeStub::CreateQuery(const char*) {
    return 0u;
}
void MonsterStorageRuntimeStub::DestroyQuery(std::uint32_t) {}
IScriptAttributeTreeRuntime& MonsterStorageRuntimeStub::TreeRuntime() {
    return tree_;
}
std::int32_t MonsterStorageRuntimeStub::ParseInt(
    std::uint32_t, std::int32_t fallback) { return fallback; }
std::uint32_t MonsterStorageRuntimeStub::AllocateObject(
    std::uint32_t) { return 0u; }
void MonsterStorageRuntimeStub::FreeObject(std::uint32_t) {}
MonsterStorageRecordState& MonsterStorageRuntimeStub::RecordAt(
    std::uint32_t) { return record_; }
std::uint8_t MonsterStorageRuntimeStub::IncomingByte() { return 0u; }
std::uint32_t MonsterStorageRuntimeStub::CStringLength(
    std::uint32_t) { return 0u; }
IStringAssignRuntime& MonsterStorageRuntimeStub::StringRuntime() {
    static StringRuntimeStub runtime;
    return runtime;
}
[[noreturn]] void MonsterStorageRuntimeStub::NullObjectDereference() {
    throw std::runtime_error("PAL4 dereferences a null storage allocation");
}
std::uint8_t MonsterStorageRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
void MonsterStorageRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
std::uint32_t MonsterStorageRuntimeStub::FreeStringStorage(
    std::uint32_t) { return 0u; }

std::uint32_t MonsterStorageObject::InitializeRecord(
    MonsterStorageRecordState& record, std::uint32_t address,
    std::uint8_t incoming_byte) {
    record.flags = (record.flags & 0xffffff00u) | incoming_byte;
    record.text_data = 0u;
    record.text_length = 0u;
    record.text_capacity = 0u;
    record.vtable = 0x842d68u;
    return address;
}

std::uint32_t MonsterStorageObject::AllocateAndInitialize(
    IMonsterStorageRuntime& runtime) {
    const auto allocation = runtime.AllocateObject(0x18u);
    if (allocation == 0u)
        return 0u;
    auto& record = runtime.RecordAt(allocation);
    try {
        return InitializeRecord(record, allocation, runtime.IncomingByte());
    } catch (...) {
        runtime.FreeObject(allocation);
        throw;
    }
}

std::int32_t IMonsterStorageRuntime::StorageRequirement(const std::uint32_t document) {
    const auto query = CreateQuery("PAL4_MST_NEED_STORAGE");
    std::uint32_t found = 0u;
    try {
        (void)ScriptAttributeTreeLookup::Find(
            document + 0x14u, found, query, TreeRuntime());
    } catch (...) {
        DestroyQuery(query);
        throw;
    }
    DestroyQuery(query);

    const auto header = ReadDword(document + 0x18u);
    return found != header && found != 0xffffffe4u ? ParseInt(found + 0x1cu, 0) : 0;
}

std::optional<MonsterStorageValues> MonsterStorageObject::CaptureValues(
    const MonsterBehaviorManagerState& manager, IMonsterStorageCaptureRuntime& runtime) {
    const auto source = manager.words[0x654u / 4u];
    if (!source || runtime.StorageRequirement(runtime.ReadDword(source+0x4cu))<=0)
        return std::nullopt;
    return MonsterStorageValues{manager.words[1u],manager.words[0x1f0u/4u]};
}

std::uint32_t MonsterStorageObject::Create(
    const MonsterBehaviorManagerState& manager, std::uint32_t,
    IMonsterStorageRuntime& runtime) {
    const auto values=CaptureValues(manager,runtime);
    if (!values) return 0u;

    const auto allocation = AllocateAndInitialize(runtime);
    if (allocation == 0u)
        runtime.NullObjectDereference();
    auto& record = runtime.RecordAt(allocation);

    const auto name = values->name;
    const auto length = runtime.CStringLength(name);
    StringBuffer buffer({record.text_data, record.text_length,
                         record.text_capacity});
    const auto needs_copy = buffer.AllocateText(
        length, 1u, runtime.StringRuntime());
    auto text = buffer.State();
    if (needs_copy) {
        runtime.StringRuntime().Copy(text.data, name, length);
        runtime.StringRuntime().SetTextSize(text, length);
    }
    record.text_data = text.data;
    record.text_length = text.length;
    record.text_capacity = text.capacity;
    record.property = values->property;
    return allocation;
}

std::uint32_t MonsterStorageObject::Destroy(
    MonsterStorageRecordState& record, IMonsterStorageRuntime& runtime) {
    record.vtable = 0x842d68u;
    auto result = record.text_data;
    if (result != 0u) {
        const auto header = result - 1u;
        const auto reference = runtime.ReadByte(header);
        result = header;
        if (reference == 0u || reference == 0xffu)
            result = runtime.FreeStringStorage(header);
        else
            runtime.WriteByte(header,
                static_cast<std::uint8_t>(reference - 1u));
    }
    record.text_data = 0u;
    record.text_length = 0u;
    record.text_capacity = 0u;
    record.vtable = 0x841e40u;
    return result;
}

}  // namespace pal4
