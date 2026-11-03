#include "pal4/portable/gles_renderer.h"
#include "pal4/portable/scene_multisample.h"
#include "pal4/portable/effect_geometry.h"
#include <stdexcept>
#include <cmath>

namespace pal4::portable {
void VerifyEffectGeometryContract(){
    const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    Scene triangle;triangle.saturation={1,1,1};triangle.draws={{0,3,-1,-1,{1,0,0,.5f}}};
    for(const auto& p:{std::array<float,3>{-1,-1,0},{1,-1,0},{0,1,0}})triangle.vertices.push_back({{p[0],p[1],p[2]},{0,0},{0,0},{1,1,1,1}});
    GlesRenderer renderer,cylinder;
    try{
        renderer.Upload(triangle);EffectRenderState state;
        for(bool additive:{false,true}){
            GlesRenderer::BeginFrame(64,64);glClearColor(0,0,.25f,1);glClear(GL_COLOR_BUFFER_BIT);glEnable(GL_CULL_FACE);glDisable(GL_BLEND);glDepthMask(GL_TRUE);
            glBlendFuncSeparate(GL_ONE,GL_ZERO,GL_ZERO,GL_ONE);glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT,GL_FUNC_SUBTRACT);
            state.destination=additive?EffectBlendFactor::one:EffectBlendFactor::inverse_source_alpha;
            if(renderer.DrawEffect(triangle,identity,identity,state)!=1)throw std::runtime_error("effect triangle was not submitted");
            GLint src{},dst{},equation{},alpha_equation{};GLboolean write{};
            glGetIntegerv(GL_BLEND_SRC_RGB,&src);glGetIntegerv(GL_BLEND_DST_RGB,&dst);glGetIntegerv(GL_BLEND_EQUATION_RGB,&equation);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&alpha_equation);glGetBooleanv(GL_DEPTH_WRITEMASK,&write);
            if(glIsEnabled(GL_BLEND) || !glIsEnabled(GL_CULL_FACE) || !write || src!=GL_ONE || dst!=GL_ZERO || equation!=GL_FUNC_REVERSE_SUBTRACT || alpha_equation!=GL_FUNC_SUBTRACT)throw std::runtime_error("effect blend/depth/cull state leaked");
            std::array<std::uint8_t,4> pixel{};glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
            if(std::abs(int(pixel[0])-128)>2 || pixel[1]>1 || std::abs(int(pixel[2])-(additive?64:32))>2)throw std::runtime_error("effect alpha/additive blend pixel mismatch");
            // 显式不写深度的效果不能阻止随后位于其后的几何。
            auto behind=triangle;behind.draws[0].color[0]=0;behind.draws[0].color[1]=1;behind.draws[0].color[3]=1;for(auto& v:behind.vertices)v.position[2]=.2f;
            GlesRenderer back;back.Upload(behind);back.DrawScene(behind,identity,identity);back.Release();glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
            if(pixel[0]>1 || pixel[1]<254 || pixel[2]>1)throw std::runtime_error("effect depth-write=false was ignored");
        }
        // Smoke must remain visible below the world's 10% alpha cutoff.
        // Textured alpha * vertex alpha is 16/255 * .5, about 3%.
        Scene smoke=triangle;smoke.draws[0].color[3]=1;
        smoke.draws[0].diffuse=0;smoke.textures={{1,1,{255,255,255,16},true}};
        for(auto& v:smoke.vertices)v.color[3]=.5f;
        GlesRenderer soft;
        try {
            soft.Upload(smoke);
            for(bool additive:{false,true}) {
                GlesRenderer::BeginFrame(64,64);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
                EffectRenderState fade;
                fade.destination=additive?EffectBlendFactor::one:EffectBlendFactor::inverse_source_alpha;
                soft.DrawEffect(smoke,identity,identity,fade);
                std::array<std::uint8_t,4> pixel{};glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
                if(std::abs(int(pixel[0])-8)>2 || pixel[1]>1 || pixel[2]>1)
                    throw std::runtime_error("low-alpha smoke fade was clipped");
            }
            // Returning to a world pass must restore its existing cutoff.
            GlesRenderer::BeginFrame(64,64);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
            soft.DrawScene(smoke,identity,identity);
            std::array<std::uint8_t,4> clipped{};glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,clipped.data());
            if(clipped[0] || clipped[1] || clipped[2])
                throw std::runtime_error("effect alpha cutoff leaked into world pass");
            soft.Release();
        }catch(...){soft.Release();throw;}
        CylinderShape shape;shape.height=1.6f;shape.top_radius=.8f;shape.bottom_radius=.4f;shape.centered=true;const auto mesh=BuildCylinder(shape);
        cylinder.Upload(mesh.scene);GlesRenderer::BeginFrame(64,64);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);state.blend=false;state.depth_write=true;
        if(cylinder.DrawEffect(mesh.scene,identity,identity,state)!=16)throw std::runtime_error("original cylinder GPU triangle count mismatch");
        std::array<std::uint8_t,64*64*4> pixels{};glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());unsigned colored{};
        for(unsigned i=0;i<64*64;++i)if(pixels[i*4]>100 && pixels[i*4+1]>100 && pixels[i*4+2]>100)++colored;
        if(colored<100 || glGetError()!=GL_NO_ERROR)throw std::runtime_error("original cylinder has no visible GPU pixels");
        cylinder.Release();renderer.Release();glBlendEquation(GL_FUNC_ADD);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glDepthMask(GL_TRUE);
    }catch(...){cylinder.Release();renderer.Release();throw;}
}
namespace {
void VerifySceneMultisampleContract(){
    Scene mesh;mesh.saturation={1,1,1};mesh.draws={{0,3,-1,-1,{1,1,1,1}}};
    for(const auto& p:{std::array<float,3>{-.8f,-.8f,0},{.8f,.8f,0},{.84f,.8f,0}})
        mesh.vertices.push_back({{p[0],p[1],p[2]},{0,0},{0,0},{1,1,1,1}});
    const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    GlesRenderer renderer;SceneMultisample target;
    try{
        renderer.Upload(mesh);GLint original{};glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&original);
        for(int requested:{0,4}){
            auto frame=target.Begin(64,64,requested);GlesRenderer::BeginFrame(64,64);
            glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
            renderer.DrawScene(mesh,identity,identity);frame.Resolve();
            GLint bound{};glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&bound);
            if(bound!=original)throw std::runtime_error("scene resolve lost caller framebuffer");
            std::array<std::uint8_t,64*64*4> pixels{};glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
            unsigned partial{};for(unsigned i=0;i<64*64;++i)if(pixels[i*4]>0 && pixels[i*4]<255)++partial;
            if(target.Samples()>1 && partial<10)throw std::runtime_error("thin scene geometry has no multisample coverage");
            if(requested==0 && partial)throw std::runtime_error("single-sample comparison changed coverage");
        }
        // With MSAA, a covered sample can lie inside a very thin triangle while
        // its pixel center lies outside. UV interpolation must stay in the atlas
        // island (centroid); extrapolation would reintroduce white dotted edges.
        Texture atlas{128,128,std::vector<std::uint8_t>(128*128*4,255),false,TextureFilter::linear};
        for(unsigned y=0;y<128;++y)for(unsigned x=60;x<66;++x){const auto at=(y*128+x)*4;atlas.rgba[at]=53;atlas.rgba[at+1]=31;atlas.rgba[at+2]=30;}
        mesh.textures={atlas};mesh.draws[0].diffuse=0;
        mesh.vertices[0].uv0[0]=mesh.vertices[1].uv0[0]=60.5f/128;
        mesh.vertices[2].uv0[0]=65.5f/128;
        renderer.Upload(mesh);
        {auto frame=target.Begin(64,64);GlesRenderer::BeginFrame(64,64);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
         renderer.DrawScene(mesh,identity,identity);}
        std::array<std::uint8_t,64*64*4> atlas_pixels{};glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,atlas_pixels.data());
        for(unsigned i=0;i<64*64;++i)if(atlas_pixels[i*4]>55 || atlas_pixels[i*4+1]>33 || atlas_pixels[i*4+2]>32)
            throw std::runtime_error("multisample edge extrapolated outside texture atlas");
        {auto frame=target.Begin(32,48);GlesRenderer::BeginFrame(32,48);}
        GLint bound{};glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&bound);
        if(bound!=original || glGetError()!=GL_NO_ERROR)throw std::runtime_error("scene resize/resolve failed");
        // Switch resolves to an existing 4:3 FBO, not necessarily framebuffer 0.
        struct Destination {
            GLuint buffer{},color{};GLint draw{},read{},renderbuffer{};
            Destination(){
                glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);glGetIntegerv(GL_RENDERBUFFER_BINDING,&renderbuffer);
                glGenFramebuffers(1,&buffer);glBindFramebuffer(GL_FRAMEBUFFER,buffer);
                glGenRenderbuffers(1,&color);glBindRenderbuffer(GL_RENDERBUFFER,color);glRenderbufferStorage(GL_RENDERBUFFER,GL_RGBA8,32,48);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,color);
            }
            ~Destination(){glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);glBindRenderbuffer(GL_RENDERBUFFER,renderbuffer);glDeleteRenderbuffers(1,&color);glDeleteFramebuffers(1,&buffer);}
        };
        {Destination destination;
         if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("scene test destination incomplete");
         {auto frame=target.Begin(32,48);glClearColor(.25f,.5f,.75f,1);glClear(GL_COLOR_BUFFER_BIT);}
         std::array<std::uint8_t,4> pixel{};glReadPixels(16,24,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
         if(std::abs(int(pixel[0])-64)>1 || std::abs(int(pixel[1])-128)>1 || std::abs(int(pixel[2])-191)>1)
             throw std::runtime_error("scene resolve missed nondefault destination");}
        target.Release();renderer.Release();
    }catch(...){target.Release();renderer.Release();throw;}
}
void VerifyAtlasFilteringContract(){
    // A narrow brown atlas island on white, heavily minified along V.
    // Model linear sampling must not pull white padding from generated mips.
    Scene scene;scene.saturation={1,1,1};scene.draws={{0,3,0,-1,{1,1,1,1}}};
    Texture atlas{128,128,std::vector<std::uint8_t>(128*128*4,255),false};
    for(unsigned y=0;y<128;++y)for(unsigned x=60;x<66;++x){
        const auto at=(y*128+x)*4;atlas.rgba[at]=53;atlas.rgba[at+1]=31;atlas.rgba[at+2]=30;
    }
    scene.textures.push_back(atlas);
    for(const auto& p:{std::array<float,3>{-1,-1,0},{1,-1,0},{0,1,0}})
        scene.vertices.push_back({{p[0],p[1],p[2]},{62.5f/128,(p[1]+1)*32},{0,0},{1,1,1,1}});
    const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    GlesRenderer renderer;
    try{
        for(auto filter:{TextureFilter::linear,TextureFilter::trilinear}){
            scene.textures[0].filter=filter;renderer.Upload(scene);GlesRenderer::BeginFrame(64,64);
            renderer.DrawScene(scene,identity,identity);
            std::array<std::uint8_t,4> pixel{};glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
            if(filter==TextureFilter::linear){
                if(std::abs(int(pixel[0])-53)>2 || std::abs(int(pixel[1])-31)>2 || std::abs(int(pixel[2])-30)>2)
                    throw std::runtime_error("linear model atlas sampled white mip padding");
            }else if(pixel[0]<200 || pixel[1]<200 || pixel[2]<200)
                throw std::runtime_error("trilinear texture policy was lost");
        }
        renderer.Release();
    }catch(...){renderer.Release();throw;}
}
void VerifySkyContract(){
    const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    Scene sky;sky.saturation={1,1,1};sky.draws={{0,3,-1,-1,{0,0,1,1}}};
    for(const auto& p:{std::array<float,3>{-1,-1,20000},{1,-1,20000},{0,1,20000}})
        sky.vertices.push_back({{p[0],p[1],p[2]},{0,0},{0,0},{1,1,1,1}});
    auto front=sky;for(auto& v:front.vertices)v.position[2]=0;front.draws[0].color[1]=1;front.draws[0].color[2]=0;
    GlesRenderer back,world;
    try{
        back.Upload(sky);world.Upload(front);GlesRenderer::BeginFrame(64,64);
        back.DrawSky(sky,identity);
        std::array<std::uint8_t,4> pixel{};glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
        if(pixel[0]>1 || pixel[1]>1 || pixel[2]<254)throw std::runtime_error("sky clipped at world far plane");
        world.DrawScene(front,identity,identity);back.DrawSky(sky,identity);
        glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
        GLboolean write{};glGetBooleanv(GL_DEPTH_WRITEMASK,&write);
        if(pixel[0]>1 || pixel[1]<254 || pixel[2]>1 || !write || glGetError()!=GL_NO_ERROR)throw std::runtime_error("sky occludes world or leaks depth state");
        back.Release();world.Release();
    }catch(...){back.Release();world.Release();throw;}
}
void VerifyDrawAppearanceContract() {
    Scene scene;scene.saturation={1,1,1};scene.draws={{0,3,-1,-1,{1,0,0,1}}};
    for(const auto& p:{std::array<float,3>{-1,-1,0},{1,-1,0},{0,1,0}})
        scene.vertices.push_back({{p[0],p[1],p[2]},{0,0},{0,0},{1,1,1,1}});
    const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    GlesRenderer renderer;
    try {
        renderer.Upload(scene);
        for(unsigned step=0;step<3;++step) {
            GlesRenderer::BeginFrame(64,64);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
            const std::array<DrawAppearance,1> appearance{{{step!=0,step==1?std::array<float,4>{.5f,.5f,.5f,.5f}:std::array<float,4>{1,1,1,1}}}};
            const auto triangles=renderer.DrawScene(scene,identity,identity,appearance);
            std::array<std::uint8_t,4> pixel{};glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
            const unsigned expected=step==0?0:step==1?64:255;
            for(unsigned c=0;c<3;++c)if(std::abs(int(pixel[c])-int(expected))>2)throw std::runtime_error("Atomic appearance GPU color/cull contract failed");
            if(triangles!=(step==0?0u:1u) || glGetError()!=GL_NO_ERROR)throw std::runtime_error("Atomic appearance GPU submission contract failed");
        }
        renderer.Release();
    }catch(...){renderer.Release();throw;}
}
}
void VerifySceneCompositionContract() {
    Scene front; front.saturation={1,1,1};
    front.draws={{0,3,0,-1,{1,1,1,1}}};
    Texture red{8,8,std::vector<std::uint8_t>(8*8*4),false};
    for(unsigned i=0;i<64;++i){red.rgba[i*4]=255;red.rgba[i*4+3]=255;}
    front.textures.push_back(red);
    for (const auto& p:{std::array<float,3>{-1,-1,0},{1,-1,0},{0,1,0}})
        front.vertices.push_back({{p[0],p[1],.4f},{0,0},{0,0},{1,1,1,1}});
    // A small dynamic effect buffer must not become the source for a large
    // world's high-offset draw after switching VAOs on a later frame.
    front.vertices.resize(3003);
    for(unsigned i=0;i<3;++i)front.vertices[3000+i]=front.vertices[i];
    front.draws[0].first=3000;
    auto back=front;for(unsigned i=0;i<64;++i){back.textures[0].rgba[i*4]=0;back.textures[0].rgba[i*4+1]=255;}
    for (auto& v:back.vertices) v.position[2]=-.4f;
    const std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    // Without model transforms green is closer. With the instance transforms
    // red is closer, so equal local-depth ties cannot hide a missing matrix.
    auto near_model=identity,far_model=identity; near_model[14]=-.8f; far_model[14]=.8f;
    GlesRenderer a,b;
    try {
        a.Upload(front); b.Upload(back); GlesRenderer::BeginFrame(64,64);
        a.DrawScene(front,identity,near_model); b.DrawScene(back,identity,far_model);
        std::array<std::uint8_t,4> pixel{};
        glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
        // Effects are uploaded lazily after the world has already drawn.
        // Reusing the world on a later frame must retain its geometry/textures.
        GlesRenderer late;
        try {
            auto small=back;small.vertices.resize(3);small.draws[0].first=0;
            late.Upload(small);
            for(unsigned frame=0;frame<3;++frame){
                GlesRenderer::BeginFrame(64,64);
                a.DrawScene(front,identity,near_model);b.DrawScene(back,identity,far_model);
                glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
                if(pixel[0]<250 || pixel[1]>5)throw std::runtime_error("lazy textured resource upload corrupted existing scene");
                late.UpdateVertices(small.vertices);EffectRenderState state;
                late.DrawEffect(small,identity,far_model,state);
            }
            late.Release();
        }catch(...){late.Release();throw;}
        const auto error=glGetError(); a.Release(); b.Release();
        if (error!=GL_NO_ERROR || pixel[0]<250 || pixel[1]>5)
            throw std::runtime_error("scene composition/model/depth GPU contract failed");
    } catch (...) { a.Release(); b.Release(); throw; }
}
std::array<std::uint8_t,4> VerifyWorldDepthContract() {
    VerifySceneCompositionContract();
    VerifyEffectGeometryContract();
    VerifySkyContract();
    VerifyAtlasFilteringContract();
    VerifySceneMultisampleContract();
    VerifyDrawAppearanceContract();
    Scene scene;
    scene.minimum={-1,-1,-1}; scene.maximum={1,1,2}; scene.saturation={1,1,1};
    scene.textures={{1,1,{255,0,0,250},true},{1,1,{0,255,0,250},true}};
    scene.draws={{0,3,0,-1,{1,1,1,1}},{3,3,1,-1,{1,1,1,1}}};
    for (const auto& position : {std::array<float,3>{-1,-1,1},{1,-1,1},{0,1,1},
                                {-1,-1,2},{1,-1,-1},{0,1,-1}})
        scene.vertices.push_back({{position[0],position[1],position[2]},{0,0},{0,0},{1,1,1,1}});
    OrbitCamera camera; camera.target={0,0,0}; camera.yaw=0; camera.pitch=0;
    camera.distance=5; camera.scale=5;
    GlesRenderer renderer;
    try {
        renderer.Upload(scene); renderer.Render(scene,camera,64,64);
        std::array<std::uint8_t,4> pixel{};
        glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
        const auto error=glGetError(); renderer.Release();
        if (error!=GL_NO_ERROR || pixel[0]<200 || pixel[1]>20)
            throw std::runtime_error("world depth GPU contract failed");
        return pixel;
    } catch (...) { renderer.Release(); throw; }
}
}  // namespace pal4::portable
