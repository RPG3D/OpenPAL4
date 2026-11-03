#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleForbiddenSkillRuntime {
public:
    virtual ~IPalConsoleForbiddenSkillRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::int32_t PlayerId(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t id) = 0;
    virtual void SetPresent(std::uint32_t collection,
                            std::int32_t skill, bool present) = 0;
    [[nodiscard]] virtual std::string_view PlayerName(
        std::uint32_t player) = 0;
    virtual void InitializeFormatBuffer(std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateIterator(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual bool IteratorDone(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::int32_t IteratorValue(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::int32_t CleanupFormatBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleForbiddenSkillCommand final {
public:
    // IDA 0x5be200.
    [[nodiscard]] static std::int32_t Execute(
        const char* command, IPalConsoleForbiddenSkillRuntime& runtime);
};

}  // namespace pal4
