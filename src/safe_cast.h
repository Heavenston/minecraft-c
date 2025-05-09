#pragma once

#include <stddef.h>
#include <stdint.h>

#define safe_to_size_t(X) _Generic((X), \
    uint64_t: safe_u64_to_size_t, \
    uint32_t: safe_u32_to_size_t, \
    uint16_t: safe_u16_to_size_t, \
    uint8_t: safe_u8_to_size_t, \
    int64_t: safe_i64_to_size_t, \
    int32_t: safe_i32_to_size_t, \
    int16_t: safe_i16_to_size_t, \
    int8_t: safe_i8_to_size_t \
)(X)

#define safe_to_u64(X) _Generic((X), \
    uint64_t: safe_u64_to_u64, \
    uint32_t: safe_u32_to_u64, \
    uint16_t: safe_u16_to_u64, \
    uint8_t: safe_u8_to_u64, \
    int64_t: safe_i64_to_u64, \
    int32_t: safe_i32_to_u64, \
    int16_t: safe_i16_to_u64, \
    int8_t: safe_i8_to_u64 \
)(X)

#define safe_to_u32(X) _Generic((X), \
    uint64_t: safe_u64_to_u32, \
    uint32_t: safe_u32_to_u32, \
    uint16_t: safe_u16_to_u32, \
    uint8_t: safe_u8_to_u32, \
    int64_t: safe_i64_to_u32, \
    int32_t: safe_i32_to_u32, \
    int16_t: safe_i16_to_u32, \
    int8_t: safe_i8_to_u32 \
)(X)

#define safe_to_u16(X) _Generic((X), \
    uint64_t: safe_u64_to_u16, \
    uint32_t: safe_u32_to_u16, \
    uint16_t: safe_u16_to_u16, \
    uint8_t: safe_u8_to_u16, \
    int64_t: safe_i64_to_u16, \
    int32_t: safe_i32_to_u16, \
    int16_t: safe_i16_to_u16, \
    int8_t: safe_i8_to_u16 \
)(X)

#define safe_to_u8(X) _Generic((X), \
    uint64_t: safe_u64_to_u8, \
    uint32_t: safe_u32_to_u8, \
    uint16_t: safe_u16_to_u8, \
    uint8_t: safe_u8_to_u8, \
    int64_t: safe_i64_to_u8, \
    int32_t: safe_i32_to_u8, \
    int16_t: safe_i16_to_u8, \
    int8_t: safe_i8_to_u8 \
)(X)

#define safe_to_i64(X) _Generic((X), \
    uint64_t: safe_u64_to_i64, \
    uint32_t: safe_u32_to_i64, \
    uint16_t: safe_u16_to_i64, \
    uint8_t: safe_u8_to_i64, \
    int64_t: safe_i64_to_i64, \
    int32_t: safe_i32_to_i64, \
    int16_t: safe_i16_to_i64, \
    int8_t: safe_i8_to_i64 \
)(X)

#define safe_to_i32(X) _Generic((X), \
    uint64_t: safe_u64_to_i32, \
    uint32_t: safe_u32_to_i32, \
    uint16_t: safe_u16_to_i32, \
    uint8_t: safe_u8_to_i32, \
    int64_t: safe_i64_to_i32, \
    int32_t: safe_i32_to_i32, \
    int16_t: safe_i16_to_i32, \
    int8_t: safe_i8_to_i32 \
)(X)

#define safe_to_i16(X) _Generic((X), \
    uint64_t: safe_u64_to_i16, \
    uint32_t: safe_u32_to_i16, \
    uint16_t: safe_u16_to_i16, \
    uint8_t: safe_u8_to_i16, \
    int64_t: safe_i64_to_i16, \
    int32_t: safe_i32_to_i16, \
    int16_t: safe_i16_to_i16, \
    int8_t: safe_i8_to_i16 \
)(X)

#define safe_to_i8(X) _Generic((X), \
    uint64_t: safe_u64_to_i8, \
    uint32_t: safe_u32_to_i8, \
    uint16_t: safe_u16_to_i8, \
    uint8_t: safe_u8_to_i8, \
    int64_t: safe_i64_to_i8, \
    int32_t: safe_i32_to_i8, \
    int16_t: safe_i16_to_i8, \
    int8_t: safe_i8_to_i8 \
)(X)

size_t safe_size_t_to_size_t(size_t val);
uint64_t safe_size_t_to_u64(size_t val);
uint32_t safe_size_t_to_u32(size_t val);
uint16_t safe_size_t_to_u16(size_t val);
uint8_t safe_size_t_to_u8(size_t val);
int64_t safe_size_t_to_i64(size_t val);
int32_t safe_size_t_to_i32(size_t val);
int16_t safe_size_t_to_i16(size_t val);
int8_t safe_size_t_to_i8(size_t val);

size_t safe_u64_to_size_t(uint64_t val);
uint64_t safe_u64_to_u64(uint64_t val);
uint32_t safe_u64_to_u32(uint64_t val);
uint16_t safe_u64_to_u16(uint64_t val);
uint8_t safe_u64_to_u8(uint64_t val);
int64_t safe_u64_to_i64(uint64_t val);
int32_t safe_u64_to_i32(uint64_t val);
int16_t safe_u64_to_i16(uint64_t val);
int8_t safe_u64_to_i8(uint64_t val);

size_t safe_u32_to_size_t(uint32_t val);
uint64_t safe_u32_to_u64(uint32_t val);
uint32_t safe_u32_to_u32(uint32_t val);
uint16_t safe_u32_to_u16(uint32_t val);
uint8_t safe_u32_to_u8(uint32_t val);
int64_t safe_u32_to_i64(uint32_t val);
int32_t safe_u32_to_i32(uint32_t val);
int16_t safe_u32_to_i16(uint32_t val);
int8_t safe_u32_to_i8(uint32_t val);

size_t safe_u16_to_size_t(uint16_t val);
uint64_t safe_u16_to_u64(uint16_t val);
uint32_t safe_u16_to_u32(uint16_t val);
uint16_t safe_u16_to_u16(uint16_t val);
uint8_t safe_u16_to_u8(uint16_t val);
int64_t safe_u16_to_i64(uint16_t val);
int32_t safe_u16_to_i32(uint16_t val);
int16_t safe_u16_to_i16(uint16_t val);
int8_t safe_u16_to_i8(uint16_t val);

size_t safe_u8_to_size_t(uint8_t val);
uint64_t safe_u8_to_u64(uint8_t val);
uint32_t safe_u8_to_u32(uint8_t val);
uint16_t safe_u8_to_u16(uint8_t val);
uint8_t safe_u8_to_u8(uint8_t val);
int64_t safe_u8_to_i64(uint8_t val);
int32_t safe_u8_to_i32(uint8_t val);
int16_t safe_u8_to_i16(uint8_t val);
int8_t safe_u8_to_i8(uint8_t val);

size_t safe_i64_to_size_t(int64_t val);
uint64_t safe_i64_to_u64(int64_t val);
uint32_t safe_i64_to_u32(int64_t val);
uint16_t safe_i64_to_u16(int64_t val);
uint8_t safe_i64_to_u8(int64_t val);
int64_t safe_i64_to_i64(int64_t val);
int32_t safe_i64_to_i32(int64_t val);
int16_t safe_i64_to_i16(int64_t val);
int8_t safe_i64_to_i8(int64_t val);

size_t safe_i32_to_size_t(int32_t val);
uint64_t safe_i32_to_u64(int32_t val);
uint32_t safe_i32_to_u32(int32_t val);
uint16_t safe_i32_to_u16(int32_t val);
uint8_t safe_i32_to_u8(int32_t val);
int64_t safe_i32_to_i64(int32_t val);
int32_t safe_i32_to_i32(int32_t val);
int16_t safe_i32_to_i16(int32_t val);
int8_t safe_i32_to_i8(int32_t val);

size_t safe_i16_to_size_t(int16_t val);
uint64_t safe_i16_to_u64(int16_t val);
uint32_t safe_i16_to_u32(int16_t val);
uint16_t safe_i16_to_u16(int16_t val);
uint8_t safe_i16_to_u8(int16_t val);
int64_t safe_i16_to_i64(int16_t val);
int32_t safe_i16_to_i32(int16_t val);
int16_t safe_i16_to_i16(int16_t val);
int8_t safe_i16_to_i8(int16_t val);

size_t safe_i8_to_size_t(int8_t val);
uint64_t safe_i8_to_u64(int8_t val);
uint32_t safe_i8_to_u32(int8_t val);
uint16_t safe_i8_to_u16(int8_t val);
uint8_t safe_i8_to_u8(int8_t val);
int64_t safe_i8_to_i64(int8_t val);
int32_t safe_i8_to_i32(int8_t val);
int16_t safe_i8_to_i16(int8_t val);
int8_t safe_i8_to_i8(int8_t val);
