#include "pal4/portable/gles_renderer.h"
#include "pal4/portable/effect_cylinder.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <stdexcept>
#include <string>

namespace pal4::portable {
namespace {
void BindVertexLayout(GLuint buffer) {
    glBindBuffer(GL_ARRAY_BUFFER,buffer);
    constexpr int sizes[]{3,2,2,4};
    constexpr std::size_t offsets[]{offsetof(Vertex,position),offsetof(Vertex,uv0),
                                  offsetof(Vertex,uv1),offsetof(Vertex,color)};
    for(GLuint index=0;index<4;++index) {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index,sizes[index],GL_FLOAT,GL_FALSE,sizeof(Vertex),
                              reinterpret_cast<const void*>(offsets[index]));
    }
}
GLuint Compile(GLenum kind, const char* source) {
    const auto shader = glCreateShader(kind);
    glShaderSource(shader,1,&source,nullptr); glCompileShader(shader);
    GLint passed{}; glGetShaderiv(shader,GL_COMPILE_STATUS,&passed);
    if (!passed) {
        char log[2048]{}; glGetShaderInfoLog(shader,sizeof log,nullptr,log);
        glDeleteShader(shader); throw std::runtime_error(std::string("shader: ")+log);
    }
    return shader;
}
constexpr auto vertex_source = R"(#version 300 es
layout(location=0) in vec3 position;
layout(location=1) in vec2 uv0;
layout(location=2) in vec2 uv1;
layout(location=3) in vec4 prelight;
uniform mat4 viewProjection;
uniform bool skyPass;
uniform mat3 diffuseTransform;
uniform mat3 lightTransform;
centroid out vec2 diffuseUV;
centroid out vec2 lightUV;
out vec4 vertexColor;
void main() {
    gl_Position=viewProjection*vec4(position,1.0);
    if(skyPass)gl_Position.z=gl_Position.w*0.99999;
    diffuseUV=(diffuseTransform*vec3(uv0,1.0)).xy;
    lightUV=(lightTransform*vec3(uv1,1.0)).xy; vertexColor=prelight;
})";
constexpr auto fragment_source = R"(#version 300 es
precision highp float;
uniform sampler2D diffuseMap;
uniform sampler2D lightMap;
uniform vec4 materialColor;
uniform vec3 saturation;
uniform float brightness;
uniform float alphaCutoff;
centroid in vec2 diffuseUV;
centroid in vec2 lightUV;
in vec4 vertexColor;
out vec4 outputColor;
void main() {
    vec4 base=vertexColor*materialColor*texture(diffuseMap,diffuseUV);
    if(base.a==0.0 || base.a<alphaCutoff) discard;
    vec4 lit=base*texture(lightMap,lightUV)*(1.0+brightness);
    outputColor=vec4(lit.rgb*saturation,lit.a);
})";
void TextureData(const Texture& texture) {
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,texture.filter==TextureFilter::linear?GL_LINEAR:GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    const auto address=texture.address==TextureAddress::clamp?GL_CLAMP_TO_EDGE:GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,address);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,address);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,texture.width,texture.height,0,
                 GL_RGBA,GL_UNSIGNED_BYTE,texture.rgba.data());
    if(texture.filter==TextureFilter::trilinear)glGenerateMipmap(GL_TEXTURE_2D);
}
GLuint TextureObject(const Texture& texture) {
    GLuint name{}; glGenTextures(1,&name); glBindTexture(GL_TEXTURE_2D,name);
    TextureData(texture);return name;
}
}
void GlesRenderer::Upload(const Scene& scene) {
    if (!glGetString(GL_VERSION)) throw std::runtime_error("GPU upload requires a current GLES context");
    Release();
    const auto vs=Compile(GL_VERTEX_SHADER,vertex_source);
    GLuint fs{};
    try { fs=Compile(GL_FRAGMENT_SHADER,fragment_source); }
    catch (...) { glDeleteShader(vs); throw; }
    program_=glCreateProgram();
    glAttachShader(program_,vs); glAttachShader(program_,fs); glLinkProgram(program_);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint passed{}; glGetProgramiv(program_,GL_LINK_STATUS,&passed);
    if (!passed) {
        char log[2048]{}; glGetProgramInfoLog(program_,sizeof log,nullptr,log);
        Release(); throw std::runtime_error(std::string("program: ")+log);
    }
    glUseProgram(program_);
    matrix_=glGetUniformLocation(program_,"viewProjection");
    sky_=glGetUniformLocation(program_,"skyPass");
    color_=glGetUniformLocation(program_,"materialColor");
    saturation_=glGetUniformLocation(program_,"saturation");
    brightness_=glGetUniformLocation(program_,"brightness");
    alpha_cutoff_=glGetUniformLocation(program_,"alphaCutoff");
    uv_transform_={glGetUniformLocation(program_,"diffuseTransform"),glGetUniformLocation(program_,"lightTransform")};
    glUniform1i(glGetUniformLocation(program_,"diffuseMap"),0);
    glUniform1i(glGetUniformLocation(program_,"lightMap"),1);
    glGenVertexArrays(1,&vao_); glBindVertexArray(vao_);
    glGenBuffers(1,&buffer_); glBindBuffer(GL_ARRAY_BUFFER,buffer_);
    glBufferData(GL_ARRAY_BUFFER,scene.vertices.size()*sizeof(Vertex),scene.vertices.data(),GL_STATIC_DRAW);
    vertex_count_=scene.vertices.size();
    BindVertexLayout(buffer_);
    Texture white{1,1,{255,255,255,255},false}; white_=TextureObject(white);
    for (const auto& texture : scene.textures) textures_.push_back(TextureObject(texture));
    const auto error=glGetError();
    if (error!=GL_NO_ERROR) { Release(); throw std::runtime_error("GPU upload error="+std::to_string(error)); }
}
void GlesRenderer::UpdateVertices(const std::vector<Vertex>& vertices) {
    if (!buffer_ || vertices.size()!=vertex_count_)
        throw std::runtime_error("dynamic vertex update does not match uploaded geometry");
    glBindBuffer(GL_ARRAY_BUFFER,buffer_);
    glBufferSubData(GL_ARRAY_BUFFER,0,vertices.size()*sizeof(Vertex),vertices.data());
}
void GlesRenderer::UpdateTexture(std::size_t index,const Texture& texture){
    if(!program_||index>=textures_.size())throw std::invalid_argument("dynamic texture index is not uploaded");
    if(!texture.width||!texture.height||texture.width>4096||texture.height>4096||texture.rgba.size()!=std::size_t(texture.width)*texture.height*4)
        throw std::invalid_argument("invalid dynamic texture dimensions");
    GLint binding{};glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);
    glBindTexture(GL_TEXTURE_2D,textures_[index]);TextureData(texture);glBindTexture(GL_TEXTURE_2D,binding);
}
void GlesRenderer::Render(const Scene& scene,const OrbitCamera& camera,int width,int height) {
    if (!program_ || width<=0 || height<=0) return;
    BeginFrame(width,height);
    const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    DrawScene(scene,camera.Matrix(static_cast<float>(width)/height),identity);
}
void GlesRenderer::BeginFrame(int width,int height) {
    glViewport(0,0,width,height);
    glClearColor(.1f,.15f,.17f,1);
    glDepthMask(GL_TRUE); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}
std::uint32_t GlesRenderer::DrawScene(const Scene& scene,const std::array<float,16>& view_projection,
                             const std::array<float,16>& model,std::span<const DrawAppearance> appearance,FaceCulling culling,float alpha_cutoff) {
    if(!std::isfinite(alpha_cutoff)||alpha_cutoff<0||alpha_cutoff>1)throw std::invalid_argument("invalid scene alpha cutoff");
    return DrawConfigured(scene,view_projection,model,appearance,nullptr,false,culling,alpha_cutoff);
}
namespace {
GLenum BlendFactor(EffectBlendFactor value){
    switch(value){
    case EffectBlendFactor::zero:return GL_ZERO;case EffectBlendFactor::one:return GL_ONE;
    case EffectBlendFactor::source_color:return GL_SRC_COLOR;case EffectBlendFactor::inverse_source_color:return GL_ONE_MINUS_SRC_COLOR;
    case EffectBlendFactor::source_alpha:return GL_SRC_ALPHA;case EffectBlendFactor::inverse_source_alpha:return GL_ONE_MINUS_SRC_ALPHA;
    case EffectBlendFactor::destination_color:return GL_DST_COLOR;case EffectBlendFactor::inverse_destination_color:return GL_ONE_MINUS_DST_COLOR;
    case EffectBlendFactor::destination_alpha:return GL_DST_ALPHA;case EffectBlendFactor::inverse_destination_alpha:return GL_ONE_MINUS_DST_ALPHA;
    }throw std::invalid_argument("unsupported effect blend factor");
}
void Enable(GLenum capability,bool enabled){if(enabled)glEnable(capability);else glDisable(capability);}
struct EffectStateRestore {
    bool active{};GLboolean blend{},depth{},cull{},write{};GLint src_rgb{},dst_rgb{},src_alpha{},dst_alpha{},equation_rgb{},equation_alpha{},front_face{},cull_face{};
    explicit EffectStateRestore(bool save):active(save){if(!active)return;
        blend=glIsEnabled(GL_BLEND);depth=glIsEnabled(GL_DEPTH_TEST);cull=glIsEnabled(GL_CULL_FACE);glGetBooleanv(GL_DEPTH_WRITEMASK,&write);
        glGetIntegerv(GL_FRONT_FACE,&front_face);glGetIntegerv(GL_CULL_FACE_MODE,&cull_face);
        glGetIntegerv(GL_BLEND_SRC_RGB,&src_rgb);glGetIntegerv(GL_BLEND_DST_RGB,&dst_rgb);glGetIntegerv(GL_BLEND_SRC_ALPHA,&src_alpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&dst_alpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&equation_rgb);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&equation_alpha);
    }
    ~EffectStateRestore(){if(!active)return;Enable(GL_BLEND,blend);Enable(GL_DEPTH_TEST,depth);Enable(GL_CULL_FACE,cull);glDepthMask(write);
        glFrontFace(front_face);glCullFace(cull_face);
        glBlendFuncSeparate(src_rgb,dst_rgb,src_alpha,dst_alpha);glBlendEquationSeparate(equation_rgb,equation_alpha);
    }
};
}
std::uint32_t GlesRenderer::DrawEffect(const Scene& scene,const std::array<float,16>& view_projection,const std::array<float,16>& model,const EffectRenderState& state,std::span<const DrawAppearance> appearance){
    (void)BlendFactor(state.source);(void)BlendFactor(state.destination);return DrawConfigured(scene,view_projection,model,appearance,&state);
}
std::uint32_t GlesRenderer::DrawSky(const Scene& scene,const std::array<float,16>& projection){
    const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    EffectRenderState state;state.depth_write=false;
    return DrawConfigured(scene,projection,identity,{},&state,true);
}
std::uint32_t GlesRenderer::DrawCylinder(const EffectCylinder& cylinder,const std::array<float,16>& view_projection){
    if(!cylinder.Visible())return 0;
    EffectRenderState state;state.destination=cylinder.Additive()?EffectBlendFactor::one:EffectBlendFactor::inverse_source_alpha;
    const auto appearance=cylinder.Appearance();return DrawEffect(cylinder.Mesh(),view_projection,cylinder.Transform(),state,appearance);
}
std::uint32_t GlesRenderer::DrawConfigured(const Scene& scene,const std::array<float,16>& view_projection,
                             const std::array<float,16>& model,std::span<const DrawAppearance> appearance,const EffectRenderState* effect,bool sky,FaceCulling culling,float alpha_cutoff) {
    if (!program_) throw std::runtime_error("drawing requires uploaded geometry");
    if(!appearance.empty() && appearance.size()!=scene.draws.size())throw std::runtime_error("GPU appearance count mismatch");
    EffectStateRestore restore(effect!=nullptr || culling!=FaceCulling::none);
    std::uint32_t triangles{};
    constexpr std::array<float,4> lightmap_color{1,1,1,1};
    auto color=[&](const Draw& draw){
        // PAL4 ltMap.fx consumes prelight * diffuse * lightmap, without
        // the stored RenderWare material colour (N01 commonly stores 150/255).
        // Explicit appearance is a caller override, not that stored colour.
        if(!appearance.empty())return appearance[&draw-scene.draws.data()].color.data();
        return draw.lightmap>=0?lightmap_color.data():draw.color;
    };
    glEnable(GL_DEPTH_TEST);Enable(GL_CULL_FACE,culling!=FaceCulling::none);
    if(culling!=FaceCulling::none){glCullFace(GL_BACK);glFrontFace(culling==FaceCulling::clockwise?GL_CCW:GL_CW);}
    glUseProgram(program_); glBindVertexArray(vao_);
    // Reestablish this mesh after lazy effect uploads and other renderer passes.
    BindVertexLayout(buffer_);
    std::array<float,16> matrix{};
    for (int column=0;column<4;++column) for (int row=0;row<4;++row)
        for (int k=0;k<4;++k) matrix[column*4+row]+=view_projection[k*4+row]*model[column*4+k];
    glUniformMatrix4fv(matrix_,1,GL_FALSE,matrix.data());
    glUniform1i(sky_,sky?1:0);
    // RenderWare D3D9 defaults to alpha > 0. Soft effects must retain
    // their low-alpha fade tails; keep the existing world cutoff separately.
    glUniform1f(alpha_cutoff_,effect && !sky?0.f:alpha_cutoff);
    glUniform3fv(saturation_,1,scene.saturation.data());
    glUniform1f(brightness_,scene.brightness);
    std::array<UvTransform,2> current_uv{kIdentityUv,kIdentityUv};
    constexpr std::array<float,9> identity_uv{1,0,0,0,1,0,0,0,1};
    for(auto location:uv_transform_)glUniformMatrix3fv(location,1,GL_FALSE,identity_uv.data());
    auto transparent=[&](const Draw& draw) {
        return color(draw)[3]<1.f || (draw.diffuse>=0 && scene.textures[draw.diffuse].alpha);
    };
    std::vector<const Draw*> alpha;
    auto submit=[&](const Draw& draw) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,draw.diffuse>=0?textures_[draw.diffuse]:white_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D,draw.lightmap>=0?textures_[draw.lightmap]:white_);
        if (draw.lightmap>=0) {
            glUniform3fv(saturation_,1,scene.saturation.data());
            glUniform1f(brightness_,scene.brightness);
        } else {
            glUniform3f(saturation_,1,1,1);
            glUniform1f(brightness_,0);
        }
        glUniform4fv(color_,1,color(draw));
        for(unsigned channel=0;channel<2;++channel){
            const auto& uv=appearance.empty()?kIdentityUv:appearance[&draw-scene.draws.data()].uv[channel];
            if(uv==current_uv[channel])continue;
            const std::array<float,9> matrix{uv[0],uv[1],0,uv[2],uv[3],0,uv[4],uv[5],1};
            glUniformMatrix3fv(uv_transform_[channel],1,GL_FALSE,matrix.data());
            current_uv[channel]=uv;
        }
        glDrawArrays(GL_TRIANGLES,draw.first,draw.count);
        triangles+=draw.count/3;
    };
    if(effect){
        Enable(GL_DEPTH_TEST,effect->depth_test);glDepthMask(effect->depth_write?GL_TRUE:GL_FALSE);Enable(GL_BLEND,effect->blend);
        glBlendEquation(GL_FUNC_ADD);glBlendFunc(BlendFactor(effect->source),BlendFactor(effect->destination));
        for(const auto& draw:scene.draws){if(!appearance.empty() && !appearance[&draw-scene.draws.data()].visible)continue;submit(draw);}
        return triangles;
    }
    glDisable(GL_BLEND);
    for (const auto& draw:scene.draws) {
        if(!appearance.empty() && !appearance[&draw-scene.draws.data()].visible)continue;
        if (transparent(draw)) alpha.push_back(&draw); else submit(draw);
    }
    auto depth=[&](const Draw* draw) {
        if(!appearance.empty())if(const auto key=appearance[draw-scene.draws.data()].sort_distance)return *key;
        const auto& p=scene.vertices[draw->first].position;
        return matrix[2]*p[0]+matrix[6]*p[1]+matrix[10]*p[2]+matrix[14];
    };
    std::stable_sort(alpha.begin(),alpha.end(),[&](const Draw* a,const Draw* b){return depth(a)>depth(b);});
    // Texture alpha alone does not authorize disabling depth writes. Original
    // DXT5 world textures include nearly opaque alpha, including the Q01 hut.
    // Keep world depth writes enabled until a material explicitly requests otherwise.
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_TRUE);
    for (const auto* draw:alpha) submit(*draw);
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    return triangles;
}
void GlesRenderer::Release() {
    if (program_) {
        GLint current{};glGetIntegerv(GL_CURRENT_PROGRAM,&current);
        if (static_cast<GLuint>(current)==program_) glUseProgram(0);
    }
    if (!textures_.empty()) glDeleteTextures(static_cast<GLsizei>(textures_.size()),textures_.data());
    if (white_) glDeleteTextures(1,&white_);
    if (buffer_) glDeleteBuffers(1,&buffer_);
    if (vao_) glDeleteVertexArrays(1,&vao_);
    if (program_) glDeleteProgram(program_);
    Abandon();
}
void GlesRenderer::Abandon() {
    textures_.clear(); white_=buffer_=vao_=program_=0; vertex_count_=0;
}
}  // namespace pal4::portable
