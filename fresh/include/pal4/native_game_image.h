#pragma once
#include "pal4/native_base_object_memory.h"
#include "pal4/comparison_game_heap_trace.h"
#include <memory>
namespace pal4 {
// The original factory allocates the game image independently of adapter services.
// Preserve its size, address identity and allocator ownership for pointer-key trees.
template<class T> class NativeGameImage final {
public:
    explicit NativeGameImage(NativeBaseObjectMemory& memory,
        std::source_location source=std::source_location::current()):memory_(memory) {
        ComparisonGameHeapOriginScope origin(source);
        const auto address=memory_.Allocate(sizeof(T));
        try { value_=std::construct_at(reinterpret_cast<T*>(address)); }
        catch(...) { (void)memory_.Free(address);throw; }
    }
    ~NativeGameImage(){std::destroy_at(value_);(void)memory_.Free(reinterpret_cast<std::uint32_t>(value_));}
    NativeGameImage(const NativeGameImage&)=delete;
    NativeGameImage& operator=(const NativeGameImage&)=delete;
    T& Get() noexcept {return *value_;}
private:
    NativeBaseObjectMemory& memory_;
    T* value_{};
};
}
