#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct PalConsoleMagicRecord {
    std::uint32_t category{};
    std::uint32_t maximum{};
    std::string name;
    std::string description;
};

struct PalConsoleStuntRecord {
    std::string name;
    std::string description;
};

class IPalConsoleMasterSkillRuntime {
public:
    virtual ~IPalConsoleMasterSkillRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::int32_t PlayerId(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual bool HasMagic(std::uint32_t lookup,
                                        std::int32_t id) = 0;
    [[nodiscard]] virtual PalConsoleMagicRecord FindMagic(
        std::uint32_t lookup, std::int32_t id) = 0;
    [[nodiscard]] virtual bool HasStunt(std::uint32_t lookup,
                                        std::int32_t id) = 0;
    [[nodiscard]] virtual PalConsoleStuntRecord FindStunt(
        std::uint32_t lookup, std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentValue(
        std::uint32_t collection, std::uint32_t category,
        std::int32_t id) = 0;
    virtual void AddDelta(std::uint32_t collection,
                          std::uint32_t category, std::int32_t id,
                          std::uint32_t delta) = 0;
    virtual void InsertMagic(std::uint32_t collection,
                             std::uint32_t category,
                             std::int32_t id) = 0;
    virtual void InsertStunt(std::uint32_t collection,
                             std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleMasterSkillCommand final {
public:
    // IDA 0x5bbe40, command "masterSkill".
    [[nodiscard]] static std::int32_t Execute(
        const char* command, IPalConsoleMasterSkillRuntime& runtime);
};

}  // namespace pal4
