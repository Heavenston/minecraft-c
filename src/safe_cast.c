#include "safe_cast.h"

#include <assert.h>
#include <limits.h>

// Disable specific warnings that are intentional
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconstant-conversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#pragma GCC diagnostic ignored "-Winteger-overflow"

// Simple macros for type conversion checks
#define create_safe_cast_fn(NAME, FROM, TO) TO NAME(FROM val) { \
    if (sizeof(FROM) > sizeof(TO)) { \
        TO max_val = ~(TO)0; \
        assert(val <= (FROM)max_val); \
    } \
    return (TO)val; \
}

#define create_safe_cast_fn_signed_to_unsigned(NAME, FROM, TO) TO NAME(FROM val) { \
    assert(val >= 0); \
    if (sizeof(FROM) >= sizeof(TO)) { \
        TO max_val = ~(TO)0; \
        assert(val <= (FROM)max_val); \
    } \
    return (TO)val; \
}

#define create_safe_cast_fn_unsigned_to_signed(NAME, FROM, TO) TO NAME(FROM val) { \
    /* Maximum positive value of signed type */ \
    TO max_val = ((TO)1 << (sizeof(TO) * 8 - 1)) - 1; \
    assert(val <= (FROM)max_val); \
    return (TO)val; \
}

#define create_safe_cast_fn_signed_to_signed(NAME, FROM, TO) TO NAME(FROM val) { \
    if (sizeof(FROM) > sizeof(TO)) { \
        /* Max and min values for target type */ \
        TO max_val = ((TO)1 << (sizeof(TO) * 8 - 1)) - 1; \
        TO min_val = ((TO)1 << (sizeof(TO) * 8 - 1)); \
        assert(val <= (FROM)max_val && val >= (FROM)min_val); \
    } \
    return (TO)val; \
}

// size_t to X conversions
create_safe_cast_fn(safe_size_t_to_size_t, size_t, size_t)
create_safe_cast_fn(safe_size_t_to_u64, size_t, uint64_t)
create_safe_cast_fn(safe_size_t_to_u32, size_t, uint32_t)
create_safe_cast_fn(safe_size_t_to_u16, size_t, uint16_t)
create_safe_cast_fn(safe_size_t_to_u8, size_t, uint8_t)
create_safe_cast_fn_unsigned_to_signed(safe_size_t_to_i64, size_t, int64_t)
create_safe_cast_fn_unsigned_to_signed(safe_size_t_to_i32, size_t, int32_t)
create_safe_cast_fn_unsigned_to_signed(safe_size_t_to_i16, size_t, int16_t)
create_safe_cast_fn_unsigned_to_signed(safe_size_t_to_i8, size_t, int8_t)
create_safe_cast_fn_unsigned_to_signed(safe_size_t_to_ssize_t, size_t, ssize_t)

// uint64_t to X conversions
create_safe_cast_fn(safe_u64_to_size_t, uint64_t, size_t)
create_safe_cast_fn(safe_u64_to_u64, uint64_t, uint64_t)
create_safe_cast_fn(safe_u64_to_u32, uint64_t, uint32_t)
create_safe_cast_fn(safe_u64_to_u16, uint64_t, uint16_t)
create_safe_cast_fn(safe_u64_to_u8, uint64_t, uint8_t)
create_safe_cast_fn_unsigned_to_signed(safe_u64_to_i64, uint64_t, int64_t)
create_safe_cast_fn_unsigned_to_signed(safe_u64_to_i32, uint64_t, int32_t)
create_safe_cast_fn_unsigned_to_signed(safe_u64_to_i16, uint64_t, int16_t)
create_safe_cast_fn_unsigned_to_signed(safe_u64_to_i8, uint64_t, int8_t)
create_safe_cast_fn_unsigned_to_signed(safe_u64_to_ssize_t, uint64_t, ssize_t)

// uint32_t to X conversions
create_safe_cast_fn(safe_u32_to_size_t, uint32_t, size_t)
create_safe_cast_fn(safe_u32_to_u64, uint32_t, uint64_t)
create_safe_cast_fn(safe_u32_to_u32, uint32_t, uint32_t)
create_safe_cast_fn(safe_u32_to_u16, uint32_t, uint16_t)
create_safe_cast_fn(safe_u32_to_u8, uint32_t, uint8_t)
create_safe_cast_fn_unsigned_to_signed(safe_u32_to_i64, uint32_t, int64_t)
create_safe_cast_fn_unsigned_to_signed(safe_u32_to_i32, uint32_t, int32_t)
create_safe_cast_fn_unsigned_to_signed(safe_u32_to_i16, uint32_t, int16_t)
create_safe_cast_fn_unsigned_to_signed(safe_u32_to_i8, uint32_t, int8_t)
create_safe_cast_fn_unsigned_to_signed(safe_u32_to_ssize_t, uint32_t, ssize_t)

// uint16_t to X conversions
create_safe_cast_fn(safe_u16_to_size_t, uint16_t, size_t)
create_safe_cast_fn(safe_u16_to_u64, uint16_t, uint64_t)
create_safe_cast_fn(safe_u16_to_u32, uint16_t, uint32_t)
create_safe_cast_fn(safe_u16_to_u16, uint16_t, uint16_t)
create_safe_cast_fn(safe_u16_to_u8, uint16_t, uint8_t)
create_safe_cast_fn_unsigned_to_signed(safe_u16_to_i64, uint16_t, int64_t)
create_safe_cast_fn_unsigned_to_signed(safe_u16_to_i32, uint16_t, int32_t)
create_safe_cast_fn_unsigned_to_signed(safe_u16_to_i16, uint16_t, int16_t)
create_safe_cast_fn_unsigned_to_signed(safe_u16_to_i8, uint16_t, int8_t)
create_safe_cast_fn_unsigned_to_signed(safe_u16_to_ssize_t, uint16_t, ssize_t)

// uint8_t to X conversions
create_safe_cast_fn(safe_u8_to_size_t, uint8_t, size_t)
create_safe_cast_fn(safe_u8_to_u64, uint8_t, uint64_t)
create_safe_cast_fn(safe_u8_to_u32, uint8_t, uint32_t)
create_safe_cast_fn(safe_u8_to_u16, uint8_t, uint16_t)
create_safe_cast_fn(safe_u8_to_u8, uint8_t, uint8_t)
create_safe_cast_fn_unsigned_to_signed(safe_u8_to_i64, uint8_t, int64_t)
create_safe_cast_fn_unsigned_to_signed(safe_u8_to_i32, uint8_t, int32_t)
create_safe_cast_fn_unsigned_to_signed(safe_u8_to_i16, uint8_t, int16_t)
create_safe_cast_fn_unsigned_to_signed(safe_u8_to_i8, uint8_t, int8_t)
create_safe_cast_fn_unsigned_to_signed(safe_u8_to_ssize_t, uint8_t, ssize_t)

// int64_t to X conversions
create_safe_cast_fn_signed_to_unsigned(safe_i64_to_size_t, int64_t, size_t)
create_safe_cast_fn_signed_to_unsigned(safe_i64_to_u64, int64_t, uint64_t)
create_safe_cast_fn_signed_to_unsigned(safe_i64_to_u32, int64_t, uint32_t)
create_safe_cast_fn_signed_to_unsigned(safe_i64_to_u16, int64_t, uint16_t)
create_safe_cast_fn_signed_to_unsigned(safe_i64_to_u8, int64_t, uint8_t)
create_safe_cast_fn_signed_to_signed(safe_i64_to_i64, int64_t, int64_t)
create_safe_cast_fn_signed_to_signed(safe_i64_to_i32, int64_t, int32_t)
create_safe_cast_fn_signed_to_signed(safe_i64_to_i16, int64_t, int16_t)
create_safe_cast_fn_signed_to_signed(safe_i64_to_i8, int64_t, int8_t)
create_safe_cast_fn_signed_to_signed(safe_i64_to_ssize_t, int64_t, ssize_t)

// int32_t to X conversions
create_safe_cast_fn_signed_to_unsigned(safe_i32_to_size_t, int32_t, size_t)
create_safe_cast_fn_signed_to_unsigned(safe_i32_to_u64, int32_t, uint64_t)
create_safe_cast_fn_signed_to_unsigned(safe_i32_to_u32, int32_t, uint32_t)
create_safe_cast_fn_signed_to_unsigned(safe_i32_to_u16, int32_t, uint16_t)
create_safe_cast_fn_signed_to_unsigned(safe_i32_to_u8, int32_t, uint8_t)
create_safe_cast_fn_signed_to_signed(safe_i32_to_i64, int32_t, int64_t)
create_safe_cast_fn_signed_to_signed(safe_i32_to_i32, int32_t, int32_t)
create_safe_cast_fn_signed_to_signed(safe_i32_to_i16, int32_t, int16_t)
create_safe_cast_fn_signed_to_signed(safe_i32_to_i8, int32_t, int8_t)
create_safe_cast_fn_signed_to_signed(safe_i32_to_ssize_t, int32_t, ssize_t)

// int16_t to X conversions
create_safe_cast_fn_signed_to_unsigned(safe_i16_to_size_t, int16_t, size_t)
create_safe_cast_fn_signed_to_unsigned(safe_i16_to_u64, int16_t, uint64_t)
create_safe_cast_fn_signed_to_unsigned(safe_i16_to_u32, int16_t, uint32_t)
create_safe_cast_fn_signed_to_unsigned(safe_i16_to_u16, int16_t, uint16_t)
create_safe_cast_fn_signed_to_unsigned(safe_i16_to_u8, int16_t, uint8_t)
create_safe_cast_fn_signed_to_signed(safe_i16_to_i64, int16_t, int64_t)
create_safe_cast_fn_signed_to_signed(safe_i16_to_i32, int16_t, int32_t)
create_safe_cast_fn_signed_to_signed(safe_i16_to_i16, int16_t, int16_t)
create_safe_cast_fn_signed_to_signed(safe_i16_to_i8, int16_t, int8_t)
create_safe_cast_fn_signed_to_signed(safe_i16_to_ssize_t, int16_t, ssize_t)

// int8_t to X conversions
create_safe_cast_fn_signed_to_unsigned(safe_i8_to_size_t, int8_t, size_t)
create_safe_cast_fn_signed_to_unsigned(safe_i8_to_u64, int8_t, uint64_t)
create_safe_cast_fn_signed_to_unsigned(safe_i8_to_u32, int8_t, uint32_t)
create_safe_cast_fn_signed_to_unsigned(safe_i8_to_u16, int8_t, uint16_t)
create_safe_cast_fn_signed_to_unsigned(safe_i8_to_u8, int8_t, uint8_t)
create_safe_cast_fn_signed_to_signed(safe_i8_to_i64, int8_t, int64_t)
create_safe_cast_fn_signed_to_signed(safe_i8_to_i32, int8_t, int32_t)
create_safe_cast_fn_signed_to_signed(safe_i8_to_i16, int8_t, int16_t)
create_safe_cast_fn_signed_to_signed(safe_i8_to_i8, int8_t, int8_t)
create_safe_cast_fn_signed_to_signed(safe_i8_to_ssize_t, int8_t, ssize_t)

// ssize_t to X conversions
create_safe_cast_fn_signed_to_unsigned(safe_ssize_t_to_size_t, ssize_t, size_t)
create_safe_cast_fn_signed_to_unsigned(safe_ssize_t_to_u64, ssize_t, uint64_t)
create_safe_cast_fn_signed_to_unsigned(safe_ssize_t_to_u32, ssize_t, uint32_t)
create_safe_cast_fn_signed_to_unsigned(safe_ssize_t_to_u16, ssize_t, uint16_t)
create_safe_cast_fn_signed_to_unsigned(safe_ssize_t_to_u8, ssize_t, uint8_t)
create_safe_cast_fn_signed_to_signed(safe_ssize_t_to_i64, ssize_t, int64_t)
create_safe_cast_fn_signed_to_signed(safe_ssize_t_to_i32, ssize_t, int32_t)
create_safe_cast_fn_signed_to_signed(safe_ssize_t_to_i16, ssize_t, int16_t)
create_safe_cast_fn_signed_to_signed(safe_ssize_t_to_i8, ssize_t, int8_t)
create_safe_cast_fn_signed_to_signed(safe_ssize_t_to_ssize_t, ssize_t, ssize_t)
