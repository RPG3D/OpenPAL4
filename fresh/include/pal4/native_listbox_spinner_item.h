#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
class NativeListboxSpinnerItem;
struct NativeSpinnerServices {
    std::function<void(std::uint32_t item)> changed;
    std::function<void(std::uint32_t key,std::uint32_t row,std::uint32_t column)> event;
};
class NativeNumericSpinnerPool final {
public:
    NativeNumericSpinnerPool(NativeCeguiUiSession&,const std::filesystem::path&,NativeSpinnerServices);
    ~NativeNumericSpinnerPool();
    NativeNumericSpinnerPool(const NativeNumericSpinnerPool&)=delete;
    NativeNumericSpinnerPool& operator=(const NativeNumericSpinnerPool&)=delete;
    std::unique_ptr<NativeListboxSpinnerItem> Create(std::int32_t initial=0);
    std::int32_t Value(std::uint32_t item);
    void SetValue(std::uint32_t item,std::int32_t value);
    void RethrowEventError();
    std::size_t IdleCount()const;
    std::size_t OutstandingAllocations()const;
    // All list items must be detached/released before pool/DLL retirement.
    void Close();
private:
    friend class NativeListboxSpinnerItem;
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
class NativeListboxSpinnerItem final {
public:
    ~NativeListboxSpinnerItem();
    NativeListboxSpinnerItem(const NativeListboxSpinnerItem&)=delete;
    NativeListboxSpinnerItem& operator=(const NativeListboxSpinnerItem&)=delete;
    std::uint32_t Address()const noexcept;
    void Bind(std::uint32_t list,std::uint32_t row);
    // Original game product/sequence key, separate from the DLL row ID.
    void SetDataKey(std::uint32_t key);
    void SetRange(std::int32_t minimum,std::int32_t maximum);
    std::int32_t Value();
    void SetValue(std::int32_t);
    void Layout(const std::array<float,4>& rect,const std::array<float,4>& clip);
    // Detach/reset the list before releasing this borrowed, non-auto-delete item.
    void Close();
private:
    friend class NativeNumericSpinnerPool;
    NativeListboxSpinnerItem(std::shared_ptr<NativeNumericSpinnerPool::Runtime>,std::uint32_t);
    std::shared_ptr<NativeNumericSpinnerPool::Runtime> runtime_;
    std::uint32_t address_{};
};
}
