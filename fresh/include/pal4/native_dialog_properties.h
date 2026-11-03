#pragma once
#include "pal4/dialog_object.h"
#include <functional>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
// Host STL owns names; facet handles are borrowed. A typed reader replaces the
// original virtual+4 call, since native actor images retain identity vtables.
// Configured target nodes are borrowed from the existing Dialog config owner.
class NativeDialogProperties final {
public:
    using Reader=std::function<std::array<float,3>(std::uint32_t)>;
    NativeDialogProperties(NativeBaseObjectMemory&,DialogObjectState&,std::uint32_t target_nil);
    ~NativeDialogProperties();
    NativeDialogProperties(const NativeDialogProperties&)=delete;
    NativeDialogProperties& operator=(const NativeDialogProperties&)=delete;
    void SetFacet(std::string_view,std::uint32_t);
    bool GetVector(std::string_view,std::array<float,3>&,const Reader&);
    bool GetStoredVector(std::string_view,std::array<float,3>&);
    [[nodiscard]] std::size_t FacetCount() const noexcept;
    void Clear() noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
