#pragma once

#include "pal4/mode_string_buffer.h"
#include "pal4/native_string_buffer_runtime.h"

#include <string_view>

namespace pal4 {

// Owned VC6 name/path records for one effect-file load. The path is released
// before the script scope, while the name survives until the load returns.
class NativeEffectLoadText final {
public:
    NativeEffectLoadText(IStringBufferStorageRuntime& memory,std::string_view name);
    ~NativeEffectLoadText();
    NativeEffectLoadText(const NativeEffectLoadText&)=delete;
    NativeEffectLoadText& operator=(const NativeEffectLoadText&)=delete;
    [[nodiscard]] std::string_view Name() const noexcept;
    [[nodiscard]] std::uint32_t NameRecordAddress() const noexcept;
    [[nodiscard]] std::string_view BuildPath(std::string_view directory);
    void ReleasePath() noexcept;
private:
    NativeStringBufferRuntime strings_;
    ModeStringBufferRecord name_{};
    StringBufferState path_{};
};
} // namespace pal4
