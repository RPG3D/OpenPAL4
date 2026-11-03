#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDataClassFieldsRuntime {
public:
    virtual ~IGameDataClassFieldsRuntime() = default;
    [[nodiscard]] virtual bool InitializeDirectText(
        std::uint32_t slot, std::uint8_t seed, std::string_view text) = 0;
    virtual void ResetText(std::uint32_t slot, std::uint8_t seed,
                           std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AssignAllocatedText(
        std::uint32_t slot, std::string_view text) = 0;
    virtual void CreateEffect(std::uint32_t object, std::uint32_t slot,
                              std::uint32_t member_offset) = 0;
    virtual void ReleaseText(std::uint32_t slot) = 0;
    virtual void SetErrorFlag(std::uint32_t slot, std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class GameDataClassFields final {
public:
    // IDA 0x4d1dd0: register ID, CLASS and TYPE Effect fields.
    [[nodiscard]] static std::uint8_t Initialize(
        std::uint32_t object, IGameDataClassFieldsRuntime& runtime);
    // IDA 0x4d1fa0: two unsigned member bounds.
    [[nodiscard]] static bool MembersInRange(
        std::uint32_t object, IGameDataClassFieldsRuntime& runtime);
};

}  // namespace pal4
