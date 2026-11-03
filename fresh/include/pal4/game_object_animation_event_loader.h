#pragma once

#include "pal4/game_object_animation_event_data.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class IGameObjectAnimationEventRuntime {
public:
    virtual ~IGameObjectAnimationEventRuntime() = default;
    [[nodiscard]] virtual std::string_view RecordBasePath(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string_view RecordName(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t OpenStream(
        std::uint32_t mode, std::uint32_t flags, std::string_view path) = 0;
    virtual void ReadStream(std::uint32_t stream, std::span<std::byte> bytes) = 0;
    virtual void CloseStream(std::uint32_t stream, std::uint32_t flags) = 0;
    virtual void ClearEvents(std::uint32_t record) = 0;
    virtual void InsertEvent(
        std::uint32_t record, const AnimationEventData& event) = 0;
    [[nodiscard]] virtual bool LoadSidecar(
        std::uint32_t record, std::string_view path) = 0;
};

class GameObjectAnimationEventLoader final {
public:
    static constexpr std::string_view kSignature =
        "_pal4__Actor__Action_Message_File__(__SoftStar_sh__)__v_0_1_";

    [[nodiscard]] static bool Load(
        std::uint32_t record, IGameObjectAnimationEventRuntime& runtime);
};

}  // namespace pal4
