#include "pal4/portable/gles_overlay.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pal4::portable {
namespace {
GLuint Shader(GLenum type,const char* source) {
    const auto shader=glCreateShader(type);glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);
    GLint valid{};glGetShaderiv(shader,GL_COMPILE_STATUS,&valid);
    if(!valid){glDeleteShader(shader);throw std::runtime_error("screen overlay shader compilation failed");}
    return shader;
}
}
void GlesOverlay::Initialize() {
    Release();
    const char* vertex=R"(#version 300 es
        void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.0-1.0,0.0,1.0);})";
    const char* fragment=R"(#version 300 es
        precision highp float;uniform vec4 overlayColor;out vec4 pixel;void main(){pixel=overlayColor;})";
    const auto vs=Shader(GL_VERTEX_SHADER,vertex);GLuint fs{};
    try {
        fs=Shader(GL_FRAGMENT_SHADER,fragment);program_=glCreateProgram();
        glAttachShader(program_,vs);glAttachShader(program_,fs);glLinkProgram(program_);
        GLint valid{};glGetProgramiv(program_,GL_LINK_STATUS,&valid);
        if(!valid)throw std::runtime_error("screen overlay program link failed");
        color_=glGetUniformLocation(program_,"overlayColor");glGenVertexArrays(1,&vao_);
    } catch(...) {glDeleteShader(vs);if(fs)glDeleteShader(fs);Release();throw;}
    glDeleteShader(vs);glDeleteShader(fs);
}
void GlesOverlay::Draw(std::array<std::uint8_t,3> color,float alpha) {
    if(!std::isfinite(alpha))throw std::invalid_argument("nonfinite overlay alpha");
    if(!program_ || !vao_)throw std::logic_error("screen overlay requires a current initialized context");
    const auto blend=glIsEnabled(GL_BLEND),depth=glIsEnabled(GL_DEPTH_TEST),scissor=glIsEnabled(GL_SCISSOR_TEST),cull=glIsEnabled(GL_CULL_FACE);
    GLboolean depth_write{};glGetBooleanv(GL_DEPTH_WRITEMASK,&depth_write);
    GLint program{},vao{},source_rgb{},target_rgb{},source_alpha{},target_alpha{},equation_rgb{},equation_alpha{};
    glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
    glGetIntegerv(GL_BLEND_SRC_RGB,&source_rgb);glGetIntegerv(GL_BLEND_DST_RGB,&target_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&source_alpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&target_alpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,&equation_rgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&equation_alpha);
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_);glBindVertexArray(vao_);
    // Original Draw converts the 0..255 alpha to an integer before submission.
    const auto byte=static_cast<unsigned>(std::clamp(alpha,0.f,255.f));
    glUniform4f(color_,color[0]/255.f,color[1]/255.f,color[2]/255.f,byte/255.f);glDrawArrays(GL_TRIANGLES,0,3);
    glUseProgram(static_cast<GLuint>(program));glBindVertexArray(static_cast<GLuint>(vao));
    glBlendEquationSeparate(equation_rgb,equation_alpha);glBlendFuncSeparate(source_rgb,target_rgb,source_alpha,target_alpha);
    glDepthMask(depth_write);
    const auto restore=[](GLenum cap,GLboolean enabled){if(enabled)glEnable(cap);else glDisable(cap);};
    restore(GL_BLEND,blend);restore(GL_DEPTH_TEST,depth);restore(GL_SCISSOR_TEST,scissor);restore(GL_CULL_FACE,cull);
}
void GlesOverlay::Release() {if(program_)glDeleteProgram(program_);if(vao_)glDeleteVertexArrays(1,&vao_);Abandon();}
void GlesOverlay::Abandon() {program_=vao_=0;color_=0;}
void VerifyScreenOverlayContract() {
    GlesOverlay overlay;
    try {
        overlay.Initialize();glUseProgram(0);glBindVertexArray(0);glViewport(0,0,64,64);glDisable(GL_SCISSOR_TEST);glDepthMask(GL_TRUE);
        glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glEnable(GL_SCISSOR_TEST);glScissor(0,0,1,1);
        glDisable(GL_BLEND);glEnable(GL_CULL_FACE);
        overlay.Draw({255,0,0},127.5f);
        std::array<std::uint8_t,4> pixel{};glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
        GLboolean depth_write{};glGetBooleanv(GL_DEPTH_WRITEMASK,&depth_write);
        const bool restored=glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_SCISSOR_TEST) && glIsEnabled(GL_CULL_FACE) && !glIsEnabled(GL_BLEND) && depth_write;
        glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);
        if(!restored || pixel[0]<125 || pixel[0]>129 || pixel[1]>2 || pixel[2]<126 || pixel[2]>130)
            throw std::runtime_error("screen overlay blend/state GPU contract failed");
        overlay.Draw({0,0,0},255);glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
        if(pixel[0]>1 || pixel[1]>1 || pixel[2]>1 || glGetError()!=GL_NO_ERROR)
            throw std::runtime_error("screen overlay full-black GPU contract failed");
        overlay.Release();
    } catch(...) {overlay.Release();throw;}
}
} // namespace pal4::portable
