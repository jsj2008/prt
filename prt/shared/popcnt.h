#include <prt/shared/basic.h>

/*
 * Intrinsics do indeed suck:
 *  http://danluu.com/assembly-intrinsics/
 *
 * x86_64, Arm64 and Arm64/32 version are available.
 * The ARM codepaths require SIMD extensions.
 *
 * x86_64
 * The implementation uses 4 times unrolled
 * loop with the `popcnt` instruction, each writing
 * to it's own destination on the stack, so that
 * it can be executed each cycle, delaying the
 * execution only 2 + n cycles per `popcnt`.
 *
 * The remaining items that are not multiple of
 * 4 are added one at a time in all implementations.
 */
uint64_t popcnt64_fast(uint64_t *p, size_t len);
