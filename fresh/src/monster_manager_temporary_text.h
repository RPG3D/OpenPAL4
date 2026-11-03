#pragma once

#include "pal4/mode_string_buffer.h"

#include <cstdint>

namespace pal4 {

template <class Runtime>
class MonsterManagerTemporaryText final {
public:
    MonsterManagerTemporaryText(ModeStringBufferRecord& text,
                                Runtime& runtime)
        : text_(text), runtime_(runtime) {}
    ~MonsterManagerTemporaryText() { Release(); }

    void Release() {
        if (!active_) return;
        active_ = false;
        const auto data = text_.buffer.data;
        if (data == 0u) return;
        const auto header = data - 1u;
        const auto count = runtime_.ReadByte(header);
        if (count == 0u || count == 0xffu) {
            runtime_.FreeTextHeader(header);
        } else {
            runtime_.WriteByte(header,
                static_cast<std::uint8_t>(count - 1u));
        }
    }

    void Disable() { active_ = false; }

private:
    ModeStringBufferRecord& text_;
    Runtime& runtime_;
    bool active_{true};
};

}  // namespace pal4
