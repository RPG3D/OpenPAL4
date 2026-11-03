#pragma once

namespace pal4 {

class CollectionStaticRegistration final {
public:
    [[nodiscard]] static const char* RegisterCastWindowName(
        const char*& destination) noexcept;
    [[nodiscard]] static const char* RegisterCastWindowNameThunk(
        const char*& destination) noexcept;
};

}  // namespace pal4
