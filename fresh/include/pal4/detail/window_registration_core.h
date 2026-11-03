#pragma once

#include <cstdint>

namespace pal4::detail {

class WindowRegistrationCore final {
public:
    template <typename Runtime>
    [[nodiscard]] static std::uint32_t InitializeName(
        const std::uint32_t name_pointer,
        const std::uint32_t destination, Runtime& runtime) {
        const auto name = runtime.ReadWord(name_pointer);
        runtime.WriteWord(destination, name);
        return name;
    }

    template <typename Runtime>
    [[nodiscard]] static std::uint32_t Create(
        const std::uint32_t layout_name, Runtime& runtime) {
        return Create(layout_name, 0x38u, runtime);
    }

    template <typename Runtime>
    [[nodiscard]] static std::uint32_t Create(
        const std::uint32_t layout_name, const std::uint32_t object_size,
        Runtime& runtime) {
        const auto object = runtime.Allocate(object_size);
        if (object == 0u) return 0u;
        try {
            return runtime.Construct(object, layout_name);
        } catch (...) {
            runtime.Free(object);
            throw;
        }
    }

    template <typename Runtime>
    [[nodiscard]] static std::uint32_t RegisterFactory(
        const std::uint32_t stored_name,
        const std::uint32_t factory, Runtime& runtime) {
        const auto manager = runtime.FrameManager();
        return runtime.RegisterFactory(manager, runtime.ReadWord(stored_name),
                                       factory);
    }

    template <typename Runtime>
    [[nodiscard]] static std::uint32_t RegisterHandler(
        const std::uint32_t name_pointer,
        const std::uint32_t event_key,
        const std::uint32_t return_address, Runtime& runtime) {
        const auto name = runtime.ReadWord(name_pointer);
        return InsertHandler(name, 0x843494u, event_key,
                             return_address, runtime);
    }

    template <typename Runtime>
    [[nodiscard]] static std::uint32_t RegisterCallbackHandler(
        const std::uint32_t callback,
        const std::uint32_t event_key,
        const std::uint32_t return_address, Runtime& runtime) {
        return InsertHandler(callback, 0x8435dcu, event_key,
                             return_address, runtime);
    }

private:
    template <typename Runtime>
    [[nodiscard]] static std::uint32_t InsertHandler(
        const std::uint32_t payload, const std::uint32_t vtable,
        const std::uint32_t event_key,
        const std::uint32_t return_address, Runtime& runtime) {
        const auto handler = runtime.Allocate(8u);
        if (handler != 0u) {
            runtime.WriteWord(handler + 4u, payload);
            runtime.WriteWord(handler, vtable);
        }
        try {
            runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                    event_key, handler);
        } catch (...) {
            if (handler != 0u) runtime.DestroyHandler(handler, 1u);
            throw;
        }
        if (handler != 0u) runtime.DestroyHandler(handler, 1u);
        return return_address;
    }
};

}  // namespace pal4::detail
