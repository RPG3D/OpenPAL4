#include "pal4/string_allocated_copy.h"

namespace pal4 {
namespace {

struct TemporaryBuffer {
    StringBuffer value;
    IStringBufferRuntime& runtime;
    bool released = false;

    ~TemporaryBuffer() {
        if (!released) static_cast<void>(value.Cleanup(runtime));
    }
    std::uint32_t Release() {
        const auto result = value.Cleanup(runtime);
        released = true;
        return result;
    }
};

}  // namespace

std::uint32_t StringAllocatedCopy::CopyFromCString(
    std::uint32_t& destination, std::uint32_t source) {
    TemporaryBuffer temporary{{}, strings_};
    static_cast<void>(temporary.value.InitializeFromCString(source, strings_));
    destination = strings_.Allocate(temporary.value.State().length + 1u);
    copy_.CopyCString(destination, temporary.value.State().data);
    return temporary.Release();
}

}  // namespace pal4
