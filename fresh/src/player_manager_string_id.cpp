#include "pal4/player_manager_string_id.h"

#include <cstdint>

namespace pal4 {
namespace {

class ScopedStringBuffer final {
public:
    explicit ScopedStringBuffer(IStringBufferRuntime& runtime) : runtime_(runtime) {}
    ~ScopedStringBuffer() { static_cast<void>(buffer_.Cleanup(runtime_)); }

    ScopedStringBuffer(const ScopedStringBuffer&) = delete;
    ScopedStringBuffer& operator=(const ScopedStringBuffer&) = delete;

    void Initialize(std::uint32_t source) {
        static_cast<void>(buffer_.InitializeFromCString(source, runtime_));
    }
    [[nodiscard]] const StringBuffer& Buffer() const noexcept { return buffer_; }

private:
    IStringBufferRuntime& runtime_;
    StringBuffer buffer_;
};

}  // namespace

std::uint32_t PlayerManagerStringId::Lookup(
    const std::uint32_t input_c_string, IStringBufferRuntime& runtime) {
    ScopedStringBuffer input(runtime);
    input.Initialize(input_c_string);
    for (std::uint32_t index = 0; index < kCandidatePointers.size(); ++index) {
        bool matched = false;
        {
            ScopedStringBuffer candidate(runtime);
            candidate.Initialize(runtime.ReadDword(kCandidatePointers[index]));
            matched = StringBuffer::Compare(input.Buffer(), candidate.Buffer(),
                                            runtime) == 0;
        }
        if (matched) return index;
    }
    return static_cast<std::uint32_t>(kCandidatePointers.size());
}

}  // namespace pal4
