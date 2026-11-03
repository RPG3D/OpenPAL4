#pragma once
#include "pal4/script_random_talk.h"
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {
// Owns candidate text through host STL; the selected16-byte record is borrowed.
class NativeScriptRandomTalkPool final : private IScriptRandomTalkPoolView {
public:
    void Push(std::string_view text);
    void Clear() noexcept;
    std::uint8_t Show(IScriptTalkHandlerRuntime& talk);
    std::string_view Text(std::size_t index) const { return texts_.at(index); }
    std::size_t Size() const noexcept { return texts_.size(); }
    std::size_t Capacity() const noexcept { return texts_.capacity(); }
private:
    std::uint32_t Count() override;
    std::uint32_t Record(std::uint32_t index) override;
    std::vector<std::string> texts_;
    std::array<std::uint32_t,4> selected_{};
};
} // namespace pal4
