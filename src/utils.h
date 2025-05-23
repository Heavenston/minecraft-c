#pragma once

#include <stdint.h>
#include <stdbit.h>
#include <limits.h>
#include <__stddef_unreachable.h>

/**
 * Equivalent to C++'s `std::bit_width` function
 */
#define mcc_bit_width(x) _Generic((x), \
    unsigned char:      (UCHAR_WIDTH - stdc_leading_zeros_uc(x)), \
    unsigned short:     (USHRT_WIDTH - stdc_leading_zeros_us(x)), \
    unsigned int:       (UINT_WIDTH - stdc_leading_zeros_ui(x)), \
    unsigned long:      (ULONG_WIDTH - stdc_leading_zeros_ul(x)), \
    unsigned long long: (ULLONG_WIDTH - stdc_leading_zeros_ull(x)) \
)

#define mcc_is_po2(VAL) ((VAL & (VAL - 1)) == 0)
#define mcc_square_root(VAL) (1 << ((mcc_bit_width(VAL) - 1) >> 1))
#define mcc_next_pow_of_2(VAL) (1UL << mcc_bit_width(VAL))
#define mcc_round_up_pow_of_2(VAL) (mcc_is_pow_of_2(VAL) ? VAL : mcc_next_pow_of_2(VAL))

#define mcc_up_div(A, B) (((A) + ((B) - 1)) / (B))
#define mcc_max(A, B) ((B) > (A) ? (B) : (A))
#define mcc_min(A, B) ((B) < (A) ? (B) : (A))

#ifndef NDEBUG
#define mcc_unreachable() do { panic("Reached unreachable"); } while(false)
#else
#define mcc_unreachable() unreachable()
#endif

void panic(const char *str);

void shuffle(size_t n, size_t arr[n]);
