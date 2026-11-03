#pragma once
#include <cstddef>
extern "C" {
void* pal4_game_malloc(std::size_t);
void* pal4_game_calloc(std::size_t,std::size_t);
void pal4_game_free(void*);
void* pal4_game_realloc(void*,std::size_t);
std::size_t pal4_game_usable_size(void*);
// Target uint32 accounting; realloc deliberately retains original double-adds.
unsigned long pal4_game_accounted_bytes();
int pal4_game_disable_auto_trim();
unsigned long pal4_game_trim_threshold();
}
namespace pal4 {
// Select explicit comparison policy before either game images or SDK allocations.
void InitializeNativeGameAllocatorPolicy();
}
