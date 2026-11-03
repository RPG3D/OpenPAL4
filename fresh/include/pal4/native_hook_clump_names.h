#pragma once
#include <filesystem>
#include <memory>
#include <string_view>
#include <cstddef>

namespace pal4 {
// Owns only the name-table portion of ClumpManager. Hook clump instances and
// the singleton/container lifecycle have separate owners.
class NativeHookClumpNames final {
public:
    NativeHookClumpNames();
    ~NativeHookClumpNames();
    NativeHookClumpNames(const NativeHookClumpNames&) = delete;
    NativeHookClumpNames& operator=(const NativeHookClumpNames&) = delete;
    bool Load(const std::filesystem::path& path);
    bool Matches(std::string_view name);
    std::size_t Count() const;
private:
    struct Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
