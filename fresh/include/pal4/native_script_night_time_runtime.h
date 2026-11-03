#pragma once

#include "pal4/script_night_time_query.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class NativeGameArenaManagerOwner;

class NativeScriptNightTimeRuntime final : public IScriptNightTimeQueryRuntime {
public:
    explicit NativeScriptNightTimeRuntime(NativeGameArenaManagerOwner& arena)
        : arena_(arena) {}

    [[nodiscard]] std::uint32_t ArenaManager() override;
    [[nodiscard]] std::uint32_t ExtractTail(std::uint32_t source,
                                            std::uint32_t count) override;
    [[nodiscard]] std::uint32_t InitializeString(std::string_view value) override;
    [[nodiscard]] std::int32_t CompareCaseInsensitive(
        std::uint32_t left, std::uint32_t right) override;
    void DestroyString(std::uint32_t string) override;

private:
    NativeGameArenaManagerOwner& arena_;
    std::array<std::string, 2> strings_;
};

}  // namespace pal4
