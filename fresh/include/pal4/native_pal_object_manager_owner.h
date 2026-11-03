#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
namespace pal4 {
// Native owners supply both original virtual8/12 services and their deleting
// lifetime. They must retire any SDK resources before their dependencies close.
class INativePalObject {
public:
    virtual ~INativePalObject() = default;
    virtual std::uint32_t Address() const noexcept = 0;
    virtual void Render(std::uint32_t camera) = 0;
    virtual void Update(float elapsed) = 0;
};
// Owns the PalObject manager image and its typed objects. Host storage/locking
// use the current STL; no original private tree/lock ABI is executed.
class NativePalObjectManagerOwner final {
public:
    NativePalObjectManagerOwner();
    ~NativePalObjectManagerOwner();
    NativePalObjectManagerOwner(const NativePalObjectManagerOwner&) = delete;
    NativePalObjectManagerOwner& operator=(const NativePalObjectManagerOwner&) = delete;
    bool Register(std::string_view name, std::unique_ptr<INativePalObject> object);
    bool Remove(std::string_view name);
    std::uint32_t Find(std::string_view name) const;
    std::size_t Size() const;
    std::uint32_t RenderFrame(std::uint32_t camera);
    std::uint32_t Update(float elapsed);
    void Clear();
    void Destroy();
    std::uint32_t Address() const noexcept;
    std::size_t OutstandingAllocations() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
