#pragma once
#include <cstdint>

namespace pal4 {
class INativeBinkStateDispatch {
public:
    virtual ~INativeBinkStateDispatch() = default;
    virtual std::uint32_t Address() const = 0;
    virtual bool Enter(std::uint32_t previous_id, std::uint32_t argument) = 0;
    virtual void Leave() = 0;
    virtual void Render(std::uint32_t frame, bool obey_movie_clock) = 0;
    virtual std::uint32_t Update(float elapsed) = 0;
};

// Native state dispatcher owns the temporary empty return-to-Scene document.
class INativeBinkStateHost {
public:
    virtual ~INativeBinkStateHost() = default;
    virtual std::uint32_t CreateBinkReturnDocument() = 0;
    virtual std::uint32_t BinkStateGameAddress() const = 0;
    virtual void ReturnBinkToScene(std::uint32_t document) = 0;
    virtual std::uint32_t DestroyBinkReturnDocument(std::uint32_t document) = 0;
};
}
