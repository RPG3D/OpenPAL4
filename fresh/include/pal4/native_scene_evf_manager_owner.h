#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace pal4 {

struct NativeSceneEvfRecord;

// Owns one scene's event records and the original 52-byte manager image.
class NativeSceneEvfManagerOwner final {
public:
    NativeSceneEvfManagerOwner();
    ~NativeSceneEvfManagerOwner();
    NativeSceneEvfManagerOwner(const NativeSceneEvfManagerOwner&) = delete;
    NativeSceneEvfManagerOwner& operator=(const NativeSceneEvfManagerOwner&) = delete;

    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] std::uint32_t FindFactory(std::uint32_t type) const;
    [[nodiscard]] std::size_t FactoryCount() const noexcept;
    void AdoptRecords(std::vector<std::unique_ptr<NativeSceneEvfRecord>>&& records);
    [[nodiscard]] std::size_t RecordCount() const noexcept;
    void VisitRecords(const std::function<bool(std::uint32_t)>& visitor);
    [[nodiscard]] const NativeSceneEvfRecord* FindRecord(
        std::string_view name) const noexcept;
    [[nodiscard]] const NativeSceneEvfRecord* FindByResource(
        std::uint32_t resource) const noexcept;
    [[nodiscard]] std::uint32_t PickAtPixel(
        std::uint32_t camera, float x, float y, bool include_inactive);
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept;
    void Clear() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace pal4
