#include "pal4/player_manager_serializer.h"

#include "pal4/player_manager_object.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

void PlayerManagerSerializeRuntimeStub::InitializeFlag(
    PlayerSerializationFlag&, std::uint8_t) {}
bool PlayerManagerSerializeRuntimeStub::AllocateFlag(
    PlayerSerializationFlag&, std::uint32_t, std::uint8_t) { return false; }
void PlayerManagerSerializeRuntimeStub::CopyFlagText(
    PlayerSerializationFlag&, std::uint32_t, std::uint32_t) {}
void PlayerManagerSerializeRuntimeStub::SetFlagSize(
    PlayerSerializationFlag&, std::uint32_t) {}
void PlayerManagerSerializeRuntimeStub::DataReady(
    std::uint32_t, const PlayerSerializationFlag&) {}
void PlayerManagerSerializeRuntimeStub::FillFlag(
    std::uint32_t, PlayerSerializationFlag&) {}
std::int32_t PlayerManagerSerializeRuntimeStub::CompareFlag(
    const PlayerSerializationFlag&, std::uint32_t, std::uint32_t) { return 0; }
void PlayerManagerSerializeRuntimeStub::ProcessPlayer(
    std::uint32_t, std::uint32_t) {}
void PlayerManagerSerializeRuntimeStub::ProcessLoadedData(std::uint32_t) {}
void PlayerManagerSerializeRuntimeStub::LoadPlayer(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerSerializeRuntimeStub::CreateDocument(
    std::string_view, std::uint32_t) { return 0u; }
void PlayerManagerSerializeRuntimeStub::DestroyDocument(std::uint32_t) {}
bool PlayerManagerSerializeRuntimeStub::CheckDocumentVersion(
    std::uint32_t, std::uint32_t) { return false; }
std::uint32_t PlayerManagerSerializeRuntimeStub::FindLeadingAttribute(
    std::uint32_t, std::string_view) { return 0u; }
std::uint32_t PlayerManagerSerializeRuntimeStub::CreateAttribute(
    std::string_view, std::uint32_t) { return 0u; }
void PlayerManagerSerializeRuntimeStub::AddAttribute(
    std::uint32_t, std::uint32_t) {}
void PlayerManagerSerializeRuntimeStub::DestroyAttribute(std::uint32_t) {}
void PlayerManagerSerializeRuntimeStub::SetAttributeInt(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerSerializeRuntimeStub::AttributeToInt(
    std::uint32_t, std::uint32_t default_value) { return default_value; }
std::uint32_t PlayerManagerSerializeRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerSerializeRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint8_t PlayerManagerSerializeRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void PlayerManagerSerializeRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
void PlayerManagerSerializeRuntimeStub::Free(std::uint32_t) {}
void PlayerManagerSerializeRuntimeStub::SetVector(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
std::array<std::uint32_t, 3> PlayerManagerSerializeRuntimeStub::GetVector(
    std::uint32_t, std::uint32_t) { return {}; }
void PlayerManagerSerializeRuntimeStub::ApplyTransform(
    std::uint32_t, std::uint32_t,
    const std::array<std::uint32_t, 3>&, std::uint32_t) {}
void PlayerManagerSerializeRuntimeStub::SetPlayerState(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerSerializeRuntimeStub::CreateLookupKey(
    std::string_view, std::uint8_t&) { return 0u; }
PlayerAttributeCursor PlayerManagerSerializeRuntimeStub::FindTreeAttribute(
    std::uint32_t) { return {}; }
void PlayerManagerSerializeRuntimeStub::ResetLookupKey(std::uint32_t) {}
std::uint8_t PlayerManagerSerializeRuntimeStub::WriteDocument(
    std::uint32_t, std::uint32_t) { return 0u; }

namespace {

class ScopedFlag final {
public:
    explicit ScopedFlag(IPlayerManagerSerializeRuntime& runtime)
        : runtime_(runtime) {}
    ~ScopedFlag() {
        if (state.data == 0u) return;
        const auto header = state.data - 1u;
        const auto count = runtime_.ReadByte(header);
        if (count == 0u || count == 0xffu)
            runtime_.Free(header);
        else
            runtime_.WriteByte(header, static_cast<std::uint8_t>(count - 1u));
    }
    PlayerSerializationFlag state{};

private:
    IPlayerManagerSerializeRuntime& runtime_;
};

class ScopedDocument final {
public:
    ScopedDocument(std::uint32_t handle, IPlayerManagerSerializeRuntime& runtime)
        : handle_(handle), runtime_(runtime) {}
    ~ScopedDocument() { runtime_.DestroyDocument(handle_); }
    [[nodiscard]] std::uint32_t Handle() const noexcept { return handle_; }

private:
    std::uint32_t handle_;
    IPlayerManagerSerializeRuntime& runtime_;
};

class ScopedAttribute final {
public:
    ScopedAttribute(std::string_view name, std::uint32_t value,
                    IPlayerManagerSerializeRuntime& runtime)
        : handle_(runtime.CreateAttribute(name, value)), runtime_(runtime) {}
    ~ScopedAttribute() { runtime_.DestroyAttribute(handle_); }
    [[nodiscard]] std::uint32_t Handle() const noexcept { return handle_; }

private:
    std::uint32_t handle_;
    IPlayerManagerSerializeRuntime& runtime_;
};

class ScopedLookupKey final {
public:
    ScopedLookupKey(std::string_view name, std::uint8_t& mode_byte,
                    IPlayerManagerSerializeRuntime& runtime)
        : handle_(runtime.CreateLookupKey(name, mode_byte)), runtime_(runtime) {}
    ~ScopedLookupKey() { runtime_.ResetLookupKey(handle_); }
    [[nodiscard]] std::uint32_t Handle() const noexcept { return handle_; }

private:
    std::uint32_t handle_;
    IPlayerManagerSerializeRuntime& runtime_;
};

void AddAttribute(std::uint32_t document, std::string_view name,
                  std::uint32_t value, IPlayerManagerSerializeRuntime& runtime) {
    ScopedAttribute attribute(name, value, runtime);
    runtime.AddAttribute(document, attribute.Handle());
}

void UpsertTreeAttribute(std::uint32_t document, std::string_view name,
                         std::uint32_t value, std::uint8_t& mode_byte,
                         IPlayerManagerSerializeRuntime& runtime) {
    PlayerAttributeCursor cursor;
    {
        ScopedLookupKey key(name, mode_byte, runtime);
        cursor = runtime.FindTreeAttribute(key.Handle());
    }
    if (cursor.node != cursor.end && cursor.node != 0xffffffe4u)
        runtime.SetAttributeInt(cursor.node + 0x1cu, value);
    else
        AddAttribute(document, name, value, runtime);
}

std::uint32_t ReadTreeAttribute(std::string_view name,
                                std::uint32_t default_value,
                                std::uint8_t& mode_byte,
                                IPlayerManagerSerializeRuntime& runtime) {
    PlayerAttributeCursor cursor;
    {
        ScopedLookupKey key(name, mode_byte, runtime);
        cursor = runtime.FindTreeAttribute(key.Handle());
    }
    if (cursor.node == cursor.end || cursor.node == 0xffffffe4u)
        return default_value;
    return runtime.AttributeToInt(cursor.node + 0x1cu, default_value);
}

}  // namespace

std::uint8_t PlayerManagerSerializer::Serialize(
    const std::uint32_t manager, const std::uint32_t data,
    std::uint8_t incoming_stack_byte,
    IPlayerManagerSerializeRuntime& runtime) {
    constexpr std::string_view flag_text = "P_MGR_SERIALIZE_FLAG";
    ScopedFlag flag(runtime);
    runtime.InitializeFlag(flag.state, incoming_stack_byte);
    if (runtime.AllocateFlag(flag.state,
                             static_cast<std::uint32_t>(flag_text.size()), 1u)) {
        runtime.CopyFlagText(flag.state, kFlagSource,
                             static_cast<std::uint32_t>(flag_text.size()));
        runtime.SetFlagSize(flag.state,
                            static_cast<std::uint32_t>(flag_text.size()));
    }
    runtime.DataReady(data, flag.state);

    for (std::uint32_t i = 0u; i < PlayerManagerObject::kElementCount; ++i)
        runtime.ProcessPlayer(manager + i * PlayerManagerObject::kElementBytes,
                              data);

    const auto current = runtime.ReadDword(manager + 0x2c50u);
    const auto current_slot = manager +
        current * PlayerManagerObject::kElementBytes;
    ScopedDocument document(runtime.CreateDocument("P_MGR", 0u), runtime);
    const auto leading = runtime.ReadDword(manager + 0x2c50u);
    const auto leading_attribute = runtime.FindLeadingAttribute(
        document.Handle(), "Leading");
    if (leading_attribute != 0u)
        runtime.SetAttributeInt(leading_attribute, leading);
    else
        AddAttribute(document.Handle(), "Leading", leading, runtime);

    auto vector_name = runtime.ReadDword(kLeaderPositionName);
    runtime.SetVector(document.Handle(),
                      vector_name != 0u ? vector_name : kEmptyName,
                      current_slot + 0xacu);
    vector_name = runtime.ReadDword(kLeaderRotationName);
    runtime.SetVector(document.Handle(),
                      vector_name != 0u ? vector_name : kEmptyName,
                      current_slot + 0xa0u);

    for (std::uint32_t i = 0u; i < PlayerManagerObject::kElementCount; ++i) {
        const std::string name = "Valid" + std::to_string(i);
        const auto value = runtime.ReadDword(manager + 0x2c54u + i * 4u);
        UpsertTreeAttribute(document.Handle(), name, value,
                            incoming_stack_byte, runtime);
    }
    const auto camera_index = runtime.ReadDword(manager + 0x2c50u);
    const auto camera_value = runtime.ReadDword(manager +
        camera_index * PlayerManagerObject::kElementBytes + 0xb0cu);
    UpsertTreeAttribute(document.Handle(), "CameraAutoSeek", camera_value,
                        incoming_stack_byte, runtime);
    return runtime.WriteDocument(document.Handle(), data);
}

bool PlayerManagerSerializer::Deserialize(
    const std::uint32_t manager, const std::uint32_t data,
    std::uint8_t incoming_stack_byte,
    IPlayerManagerSerializeRuntime& runtime) {
    constexpr std::string_view flag_text = "P_MGR_SERIALIZE_FLAG";
    ScopedFlag flag(runtime);
    runtime.InitializeFlag(flag.state, incoming_stack_byte);
    runtime.FillFlag(data, flag.state);
    if (runtime.CompareFlag(flag.state, kFlagSource,
                            static_cast<std::uint32_t>(flag_text.size())) != 0)
        return false;

    runtime.ProcessLoadedData(manager);
    for (std::uint32_t i = 0u; i < PlayerManagerObject::kElementCount; ++i)
        runtime.LoadPlayer(manager + i * PlayerManagerObject::kElementBytes,
                           data);

    ScopedDocument document(runtime.CreateDocument("P_MGR", 0u), runtime);
    if (!runtime.CheckDocumentVersion(document.Handle(), data)) return false;

    const auto leading_attribute = runtime.FindLeadingAttribute(
        document.Handle(), "Leading");
    const auto leading = leading_attribute != 0u
        ? runtime.AttributeToInt(leading_attribute, 0u) : 0u;
    runtime.WriteDword(manager + 0x2c50u, leading);

    for (std::uint32_t i = 0u; i < PlayerManagerObject::kElementCount; ++i) {
        const std::string name = "Valid" + std::to_string(i);
        const auto value = ReadTreeAttribute(name, 0u,
                                              incoming_stack_byte, runtime);
        runtime.WriteDword(manager + 0x2c54u + i * 4u, value);
    }

    const auto current = runtime.ReadDword(manager + 0x2c50u);
    const auto current_slot = manager +
        current * PlayerManagerObject::kElementBytes;
    auto vector_name = runtime.ReadDword(kLeaderPositionName);
    const auto position = runtime.GetVector(document.Handle(),
        vector_name != 0u ? vector_name : kEmptyName);
    vector_name = runtime.ReadDword(kLeaderRotationName);
    const auto rotation = runtime.GetVector(document.Handle(),
        vector_name != 0u ? vector_name : kEmptyName);
    runtime.ApplyTransform(current_slot, 0x4u, position, 0u);
    runtime.ApplyTransform(current_slot, 0x8u, rotation, 0u);

    const auto camera = ReadTreeAttribute("CameraAutoSeek", 1u,
                                          incoming_stack_byte, runtime);
    runtime.SetPlayerState(manager, camera);
    return true;
}

}  // namespace pal4
