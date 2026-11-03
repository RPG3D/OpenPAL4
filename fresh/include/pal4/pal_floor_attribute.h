#pragma once

#include "pal4/cegui_text_record_array.h"
#include "pal4/floor_color_sounds.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct FloorTextRecord {
    std::uint32_t flags{};
    std::uint32_t data{};
    std::uint32_t length{};
    std::uint32_t capacity{};
};
struct FloorSoundKey {
    FloorTextRecord name{};
    std::int32_t scaled_value{};
};
struct FloorSoundRecord {
    std::uint32_t unknown{};
    FloorTextRecord name{};
    std::int32_t scaled_value{};
    FloorTextRecord sound_path{};
};
struct FloorSoundRecordCollectionState {
    std::uint32_t unknown{};
    std::uint32_t begin{};
    std::uint32_t end{};
};
struct FloorCallbackPayload {
    std::uint32_t pixel{};
    std::uint32_t context{};
    std::uint32_t callback{};
};
struct FloorCallbackNodeRecord {
    std::uint32_t next{};
    std::uint32_t previous{};
    FloorCallbackPayload payload{};
};
static_assert(sizeof(FloorTextRecord) == 16u);
static_assert(sizeof(FloorSoundKey) == 20u);
static_assert(sizeof(FloorSoundRecord) == 40u);
static_assert(sizeof(FloorSoundRecordCollectionState) == 12u);
static_assert(sizeof(FloorCallbackPayload) == 12u);
static_assert(sizeof(FloorCallbackNodeRecord) == 20u);
static_assert(offsetof(FloorCallbackNodeRecord, payload) == 8u);
static_assert(offsetof(FloorSoundRecord, name) == 4u);
static_assert(offsetof(FloorSoundRecord, scaled_value) == 20u);
static_assert(offsetof(FloorSoundRecord, sound_path) == 24u);

struct FloorColorTreeState {
    std::uint32_t flags{};
    std::uint32_t header{};
    std::uint32_t reserved{};
    std::uint32_t size{};
};
static_assert(sizeof(FloorColorTreeState) == 16u);

struct FloorColorTreeGlobals {
    std::uint32_t nil{};
    std::uint32_t owners{};
};

struct PalFloorAttributeState {
    std::uint32_t current_alpha{};
    std::uint32_t fallback_alpha{};
    std::uint32_t list_flag{};
    std::uint32_t list_head{};
    std::uint32_t list_size{};
    std::uint32_t current_color{};
    std::uint32_t previous_color{};
    FloorColorTreeState color_tree{};
    FloorTextRecord text{};
};
static_assert(sizeof(PalFloorAttributeState) == 60u);
static_assert(offsetof(PalFloorAttributeState, list_head) == 12u);
static_assert(offsetof(PalFloorAttributeState, color_tree) == 28u);
static_assert(offsetof(PalFloorAttributeState, text) == 44u);

struct PalFloorAttributeAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};


class IPalFloorAttributeRuntime : public ICeguiTextRecordRuntime {
public:
    [[nodiscard]] virtual std::int32_t FloatToLong(long double value) = 0;
    [[nodiscard]] virtual std::int32_t CompareCaseInsensitive(
        std::uint32_t left, std::uint32_t right) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destroy) = 0;
    [[nodiscard]] virtual PalFloorAttributeState* Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(PalFloorAttributeState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CreateEmptyList() = 0;
    [[nodiscard]] virtual FloorColorTreeGlobals& ColorTreeGlobals() = 0;
    [[nodiscard]] virtual std::uint32_t RgbaToPixel(std::uint32_t rgba,
                                                     std::uint32_t format) = 0;
    [[nodiscard]] virtual std::uint32_t PrepareColorValue(
        std::string_view path, std::uint32_t pixel) = 0;
    virtual void ReleaseColorValue(std::uint32_t value) = 0;
    virtual void ReleaseText(FloorTextRecord& text) = 0;
    virtual void ReleaseList(PalFloorAttributeState& state) = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    [[nodiscard]] virtual std::uint32_t CallMovementVirtual(
        std::uint32_t object, std::uint32_t slot_bytes) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateListNode(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateColorNode(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint8_t ConstructorStackByte(
        std::uint32_t index) = 0;
    virtual void AssignSoundPath(FloorTextRecord& destination,
                                 std::uint32_t source_record) = 0;
    virtual void AllocateEmptySoundPath(FloorTextRecord& destination) = 0;
    virtual void InvokeCallback(std::uint32_t function,
                                std::uint32_t context,
                                std::uint32_t pixel) = 0;
    [[nodiscard]] virtual std::uint32_t NextColorNode(
        std::uint32_t node) = 0;
    virtual void DeleteColorNode(std::uint32_t node,
                                 FloorColorTreeState& tree) = 0;
    [[nodiscard]] virtual std::uint32_t FreeAddress(std::uint32_t address) = 0;
};

class PalFloorAttributeRuntimeStub : public IPalFloorAttributeRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t EmptyTextAddress() override;
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override;
    [[nodiscard]] bool AllocateText(std::uint32_t record,
                                    std::uint32_t size,
                                    std::uint8_t mode) override;
    void DetachAndReset(std::uint32_t record) override;
    void SetTextSize(std::uint32_t record, std::uint32_t size) override;
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t size) override;
    [[noreturn]] void ThrowRange() override;
    [[nodiscard]] std::int32_t FloatToLong(long double value) override;
    [[nodiscard]] std::int32_t CompareCaseInsensitive(
        std::uint32_t left, std::uint32_t right) override;
    [[nodiscard]] std::uint32_t GameDatabase() override;
    [[nodiscard]] std::uint32_t GetSingletonManager() override;
    [[nodiscard]] std::uint32_t Register(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destroy) override;
    [[nodiscard]] PalFloorAttributeState* Allocate(std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t Free(PalFloorAttributeState* state) override;
    [[nodiscard]] std::uint32_t CreateEmptyList() override;
    [[nodiscard]] FloorColorTreeGlobals& ColorTreeGlobals() override;
    [[nodiscard]] std::uint32_t RgbaToPixel(std::uint32_t rgba,
                                             std::uint32_t format) override;
    [[nodiscard]] std::uint32_t PrepareColorValue(
        std::string_view path, std::uint32_t pixel) override;
    void ReleaseColorValue(std::uint32_t value) override;
    void ReleaseText(FloorTextRecord& text) override;
    void ReleaseList(PalFloorAttributeState& state) override;
    [[nodiscard]] std::uint32_t MovementAdapter() override;
    [[nodiscard]] std::uint32_t CallMovementVirtual(
        std::uint32_t object, std::uint32_t slot_bytes) override;
    [[nodiscard]] std::uint32_t AllocateListNode(std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t AllocateColorNode(std::uint32_t bytes) override;
    [[nodiscard]] std::uint8_t ConstructorStackByte(
        std::uint32_t index) override;
    void AssignSoundPath(FloorTextRecord& destination,
                         std::uint32_t source_record) override;
    void AllocateEmptySoundPath(FloorTextRecord& destination) override;
    void InvokeCallback(std::uint32_t function,
                        std::uint32_t context,
                        std::uint32_t pixel) override;
    [[nodiscard]] std::uint32_t NextColorNode(std::uint32_t node) override;
    void DeleteColorNode(std::uint32_t node,
                         FloorColorTreeState& tree) override;
    [[nodiscard]] std::uint32_t FreeAddress(std::uint32_t address) override;
private:
    FloorColorTreeGlobals color_tree_globals_{};
};

class FloorCallbackNode final {
public:
    [[nodiscard]] static std::uint32_t CopyPayload(
        std::uint32_t destination, const FloorCallbackPayload& source,
        IPalFloorAttributeRuntime& runtime);
};

class FloorSoundRecordCollection final {
public:
    [[nodiscard]] static std::int32_t Count(
        std::uint32_t collection, IPalFloorAttributeRuntime& runtime);
};

class PalFloorAttribute final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e4148u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e4144u;
    static constexpr std::uint32_t kFactoryAddress = 0x41fda0u;
    static constexpr std::uint32_t kDestroyAddress = 0x41fe00u;
    static constexpr std::uint32_t kEmptyErrorStringAddress = 0x8dc780u;

    [[nodiscard]] static const std::array<FloorColorSound, 15>& ColorSounds();
    [[nodiscard]] static std::int32_t ScaleValue(
        std::int32_t value, IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static bool Matches(std::uint32_t query_text,
                                      std::int32_t scaled_value,
                                      std::uint32_t record,
                                      IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t GameDatabaseData(
        std::uint32_t game_db_member, IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindSoundRecord(
        std::uint32_t query_text, std::int32_t value,
        IPalFloorAttributeRuntime& runtime);
    static void GetSoundPath(std::int32_t current, std::int32_t previous,
                             std::uint32_t query_text,
                             std::uint32_t output_text,
                             IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetSingleton(
        PalFloorAttributeAccessState& access,
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static PalFloorAttributeState* Create(
        IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        PalFloorAttributeState* state, std::uint32_t incoming_eax,
        IPalFloorAttributeRuntime& runtime);
    static void Construct(PalFloorAttributeState& state,
                          IPalFloorAttributeRuntime& runtime);
    static void Destruct(PalFloorAttributeState& state,
                         IPalFloorAttributeRuntime& runtime);
    static void DestroyColorTree(FloorColorTreeState& tree,
                                 IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateColorFromMovement(
        PalFloorAttributeState& state, IPalFloorAttributeRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddFloorAttribute(
        PalFloorAttributeState& state, std::uint32_t pixel,
        std::uint32_t context, std::uint32_t callback,
        IPalFloorAttributeRuntime& runtime);
    static void ApplyColor(PalFloorAttributeState& state,
                           std::uint32_t pixel,
                           IPalFloorAttributeRuntime& runtime);
    static void CopyData(std::uint32_t destination,
                         std::uint32_t source,
                         IPalFloorAttributeRuntime& runtime);
};

}  // namespace pal4
