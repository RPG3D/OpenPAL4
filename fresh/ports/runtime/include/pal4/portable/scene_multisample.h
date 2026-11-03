#pragma once
#include <GLES3/gl3.h>
#include <algorithm>
#include <stdexcept>

namespace pal4::portable {
// Resolve only the 3D scene, before UI/readback. The destination may be the
// Android window or Switch's fixed 4:3 presentation framebuffer.
class SceneMultisample {
public:
    class Frame {
    public:
        explicit Frame(SceneMultisample& owner):owner_(&owner){}
        Frame(const Frame&)=delete;
        Frame& operator=(const Frame&)=delete;
        ~Frame(){Resolve();}
        void Resolve(){if(owner_){owner_->Resolve();owner_=nullptr;}}
    private:SceneMultisample* owner_;
    };
    SceneMultisample()=default;
    SceneMultisample(const SceneMultisample&)=delete;
    SceneMultisample& operator=(const SceneMultisample&)=delete;
    ~SceneMultisample(){Release();}
    Frame Begin(int width,int height,int requested=0){
        if(active_ || width<=0 || height<=0 || requested<0)throw std::logic_error("invalid scene multisample frame");
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&destination_);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read_);
        if(width!=width_ || height!=height_ || requested!=requested_){
            Release();width_=width;height_=height;requested_=requested;
            GLint maximum{};glGetIntegerv(GL_MAX_SAMPLES,&maximum);
            samples_=std::min(requested,maximum);
            if(samples_>=2){
                GLint renderbuffer{};glGetIntegerv(GL_RENDERBUFFER_BINDING,&renderbuffer);
                glGenFramebuffers(1,&framebuffer_);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer_);
                glGenRenderbuffers(1,&color_);glBindRenderbuffer(GL_RENDERBUFFER,color_);
                glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples_,GL_RGBA8,width_,height_);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,color_);
                glGenRenderbuffers(1,&depth_);glBindRenderbuffer(GL_RENDERBUFFER,depth_);
                glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples_,GL_DEPTH_COMPONENT24,width_,height_);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth_);
                const bool complete=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
                glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_SAMPLES,&samples_);
                glBindRenderbuffer(GL_RENDERBUFFER,renderbuffer);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER,destination_);glBindFramebuffer(GL_READ_FRAMEBUFFER,read_);
                if(!complete){Release();throw std::runtime_error("scene multisample framebuffer incomplete");}
            }else samples_=0;
        }
        if(framebuffer_)glBindFramebuffer(GL_FRAMEBUFFER,framebuffer_);
        active_=true;return Frame(*this);
    }
    int Samples() const{return samples_;}
    void Release(){
        if(depth_)glDeleteRenderbuffers(1,&depth_);
        if(color_)glDeleteRenderbuffers(1,&color_);
        if(framebuffer_)glDeleteFramebuffers(1,&framebuffer_);
        depth_=color_=framebuffer_=0;width_=height_=requested_=samples_=0;
    }
    void Abandon(){depth_=color_=framebuffer_=0;width_=height_=requested_=samples_=0;active_=false;}
private:
    GLuint framebuffer_{},color_{},depth_{};
    GLint destination_{},read_{};
    int width_{},height_{},requested_{},samples_{};
    bool active_{};
    void Resolve(){
        if(framebuffer_){
            const auto scissor=glIsEnabled(GL_SCISSOR_TEST);glDisable(GL_SCISSOR_TEST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER,framebuffer_);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER,destination_);
            glBlitFramebuffer(0,0,width_,height_,0,0,width_,height_,GL_COLOR_BUFFER_BIT,GL_NEAREST);
            if(scissor)glEnable(GL_SCISSOR_TEST);
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,destination_);glBindFramebuffer(GL_READ_FRAMEBUFFER,read_);
        active_=false;
    }
};
}
