#pragma once
#include <array>
namespace pal4::portable {
using EffectMatrix=std::array<float,16>;
EffectMatrix IdentityEffectMatrix();
void ValidateEffectMatrix(const EffectMatrix&);
EffectMatrix MultiplyEffectMatrices(const EffectMatrix& parent,const EffectMatrix& local);
}
