#include "pal4/game_data_persistence.h"
#include "pal4/game_data_bitset_codec.h"
#include "pal4/bitset_bounds_error.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

constexpr std::string_view kMarker = "GA_SERIALIZE_FLAG";
constexpr std::string_view kRoot = "gameData";
constexpr std::string_view kTutorial = "TUTORIAL";

std::string Name(IGameDataPersistenceRuntime& runtime,
                 std::uint32_t object, std::uint32_t offset) {
    const auto address = runtime.ReadDword(object + offset);
    return address == 0u ? std::string() :
                           std::string(runtime.ReadCString(address));
}

std::string DataIntName(std::uint32_t index) {
    return "dataInt" + std::to_string(index);
}

void UpsertInt(IGameDataPersistenceRuntime& runtime, std::uint32_t node,
               std::string_view key, std::int32_t value) {
    if (runtime.HasAttribute(node, key))
        runtime.SetInt(node, key, value);
    else
        runtime.AddInt(node, key, value);
}

void UpsertString(IGameDataPersistenceRuntime& runtime, std::uint32_t node,
                  std::string_view key, std::string_view value) {
    if (runtime.HasAttribute(node, key))
        runtime.SetString(node, key, value);
    else
        runtime.AddString(node, key, value);
}

std::string TutorialBits(IGameDataPersistenceRuntime& runtime,
                         std::uint32_t object) {
    std::array<std::uint32_t, 4> words{};
    for (std::uint32_t i = 0u; i < words.size(); ++i)
        words[i] = runtime.ReadDword(object + 0x164u + 4u * i);
    std::string bits;
    (void)GameDataBitsetCodec::ToString(words, bits);
    return bits;
}

void ReadTutorialBits(IGameDataPersistenceRuntime& runtime,
                      std::uint32_t object, std::string_view text) {
    std::array<std::uint32_t, 4> words{};
    const auto size = std::min<std::size_t>(text.size(), 128u);
    for (std::size_t bit = 0; bit < size; ++bit) {
        const char digit = text[size - 1u - bit];
        if (digit == '1')
            words[bit / 32u] |= 1u << (bit % 32u);
        else if (digit != '0')
            BitsetBoundsError::ThrowInvalidCharacter();
    }
    for (std::uint32_t i = 0u; i < words.size(); ++i)
        runtime.WriteDword(object + 0x164u + i * 4u, words[i]);
}

void Release(IGameDataPersistenceRuntime& runtime, std::uint32_t stream,
             std::uint32_t node, bool native_records) {
    if (node != 0u) runtime.DestroyNode(node);
    if (native_records) runtime.ReleaseNativeRecords(stream);
    runtime.ReleaseMarker();
}

}  // namespace

std::uint8_t GameDataPersistence::Serialize(
    std::uint32_t target, IGameDataPersistenceRuntime& runtime) const {
    runtime.PrepareTarget(target, kMarker);
    std::uint32_t node = 0u;
    bool records_ready = false;
    try {
        const auto timer = runtime.TimerFrameDuration(runtime.UiFrameManager());
        runtime.WriteDword(address_ + 0x120u, timer);  // entry 70
        records_ready = true;
        runtime.ProcessNativeRecords(target, false);
        node = runtime.CreateNode(kRoot);

        const auto internal_id = runtime.ObjectInternalId(address_);
        const auto first_value = runtime.ReadDword(address_ + 4u) + internal_id;
        const auto first = Name(runtime, address_, 0x128u);
        UpsertInt(runtime, node, first,
                  static_cast<std::int32_t>(first_value));
        const auto second = Name(runtime, address_, 0x138u);
        UpsertInt(runtime, node, second,
                  static_cast<std::int32_t>(runtime.ReadDword(0x90dec0u)));
        const auto global = runtime.GlobalDataSubsystem();
        const auto third_value = runtime.ReadDword(global + 0x18u);
        const auto third = Name(runtime, address_, 0x148u);
        UpsertInt(runtime, node, third,
                  static_cast<std::int32_t>(third_value));
        for (std::uint32_t i = 0u; i < 71u; ++i) {
            UpsertInt(runtime, node, DataIntName(i),
                      static_cast<std::int32_t>(
                          runtime.ReadDword(address_ + 8u + 4u * i)));
        }
        UpsertString(runtime, node, kTutorial, TutorialBits(runtime, address_));
        const auto result = runtime.Write(node, target);
        Release(runtime, target, node, records_ready);
        return result;
    } catch (...) {
        Release(runtime, target, node, records_ready);
        throw;
    }
}

bool GameDataPersistence::Deserialize(
    std::uint32_t source, IGameDataPersistenceRuntime& runtime,
    IGameSystemDataRuntime& core_runtime) const {
    const auto marker = runtime.ReadMarker(source);
    if (!marker || *marker != kMarker) {
        runtime.ReleaseMarker();
        return false;
    }

    std::uint32_t node = 0u;
    bool records_ready = false;
    try {
        (void)GameSystemData(address_).InitializeCoreSystems(core_runtime);
        records_ready = true;
        runtime.ProcessNativeRecords(source, true);
        node = runtime.ParseNode(source, kRoot);
        if (!runtime.CheckVersionAndType(node, source)) {
            Release(runtime, source, node, records_ready);
            return false;
        }

        const auto first = Name(runtime, address_, 0x128u);
        runtime.WriteDword(address_ + 4u,
                           static_cast<std::uint32_t>(
                               runtime.ReadInt(node, first, 0)));
        const auto second = Name(runtime, address_, 0x138u);
        runtime.WriteDword(0x90dec0u,
                           static_cast<std::uint32_t>(
                               runtime.ReadInt(node, second, 0)));
        const auto third = Name(runtime, address_, 0x148u);
        const auto flags = static_cast<std::uint32_t>(
            runtime.ReadInt(node, third, 0));
        const auto global = runtime.GlobalDataSubsystem();
        runtime.WriteDword(global + 0x18u,
                           runtime.ReadDword(global + 0x18u) | flags);

        for (std::uint32_t i = 0u; i < 71u; ++i) {
            const auto fallback = (i >= 8u && i < 21u) || i == 26u ? 1 : 0;
            runtime.WriteDword(address_ + 8u + i * 4u,
                               static_cast<std::uint32_t>(runtime.ReadInt(
                                   node, DataIntName(i), fallback)));
        }
        for (std::uint32_t i = 0u; i < 47u; ++i) {
            const auto local_index = i < 5u ? i + 21u : i + 22u;
            runtime.WriteDword(0x90dec4u + i * 4u,
                runtime.ReadDword(address_ + 8u + local_index * 4u));
        }
        const auto pet = runtime.PetManager();
        const auto pet_show = runtime.ReadDword(address_ + 8u + 69u * 4u);
        runtime.WriteDword(pet + 0x654u, pet_show);
        ReadTutorialBits(runtime, address_,
                         runtime.ReadString(node, kTutorial, ""));
        Release(runtime, source, node, records_ready);
        return true;
    } catch (...) {
        Release(runtime, source, node, records_ready);
        throw;
    }
}

}  // namespace pal4
