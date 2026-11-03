#pragma once
#include <GLES3/gl3.h>
#include <array>
#include <cstdint>

namespace pal4::portable {
class GlesOverlay {
public:
    void Initialize();
    void Draw(std::array<std::uint8_t,3> color,float alpha);
    void Release();
    void Abandon();
private:
    GLuint program_{},vao_{};
    GLint color_{};
};
void VerifyScreenOverlayContract();
} // namespace pal4::portable
