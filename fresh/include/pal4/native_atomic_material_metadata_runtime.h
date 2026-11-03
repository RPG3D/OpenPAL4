#pragma once
#include "pal4/base_object_atomic_properties.h"
#include <cstring>
#include <string>

namespace pal4 {
// Publishes a borrowed native string through the original material path image.
// The caller owns both the string and the lifetime of the published fields.
class NativeAtomicMaterialMetadataRuntime final : public IBaseObjectAtomicRuntime {
public:
    explicit NativeAtomicMaterialMetadataRuntime(std::string& text) : text_(text) {}
    std::uint32_t ReadDword(std::uint32_t address) override {
        std::uint32_t value;
        std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value));
        return value;
    }
    std::uint32_t StringLength(std::uint32_t address) override {
        return address ? static_cast<std::uint32_t>(std::strlen(
            reinterpret_cast<const char*>(address))) : 0u;
    }
    void InitString(BaseObjectState& state, std::uint32_t offset,
        std::uint32_t source, std::uint32_t length) override {
        text_.assign(length ? reinterpret_cast<const char*>(source) : "", length);
        Publish(state, offset);
    }
    std::uint32_t AppendString(BaseObjectState& state, std::uint32_t offset,
        std::uint32_t source, std::uint32_t length) override {
        text_.append(reinterpret_cast<const char*>(source), length);
        Publish(state, offset);
        return reinterpret_cast<std::uint32_t>(&state) + offset;
    }
private:
    void Publish(BaseObjectState& state, std::uint32_t offset) {
        state.Write(offset, reinterpret_cast<std::uint32_t>(text_.c_str()));
        state.Write(offset + 4u, static_cast<std::uint32_t>(text_.size()));
        state.Write(offset + 8u, static_cast<std::uint32_t>(text_.size() + 1u));
    }
    std::string& text_;
};
}
