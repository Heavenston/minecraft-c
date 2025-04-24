#pragma once

#include <stddef.h>
#include <stdint.h>

#define safe_to_size_t(X) _Generic((X), \
    uint64_t: safe_u64_to_size_t, \
    uint32_t: safe_u32_to_size_t, \
    uint16_t: safe_u16_to_size_t, \
    uint8_t: safe_u8_to_size_t \
)(X)

#define safe_to_u64(X) _Generic((X), \
    uint64_t: safe_u64_to_u64, \
    uint32_t: safe_u32_to_u64, \
    uint16_t: safe_u16_to_u64, \
    uint8_t: safe_u8_to_u64 \
)(X)

#define safe_to_u32(X) _Generic((X), \
    uint64_t: safe_u64_to_u32, \
    uint32_t: safe_u32_to_u32, \
    uint16_t: safe_u16_to_u32, \
    uint8_t: safe_u8_to_u32 \
)(X)

#define safe_to_u16(X) _Generic((X), \
    uint64_t: safe_u64_to_u16, \
    uint32_t: safe_u32_to_u16, \
    uint16_t: safe_u16_to_u16, \
    uint8_t: safe_u8_to_u16 \
)(X)

#define safe_to_u8(X) _Generic((X), \
    uint64_t: safe_u64_to_u8, \
    uint32_t: safe_u32_to_u8, \
    uint16_t: safe_u16_to_u8, \
    uint8_t: safe_u8_to_u8 \
)(X)

size_t safe_size_t_to_size_t(size_t val);
uint64_t safe_size_t_to_u64(size_t val);
uint32_t safe_size_t_to_u32(size_t val);
uint16_t safe_size_t_to_u16(size_t val);
uint8_t safe_size_t_to_u8(size_t val);

size_t safe_u64_to_size_t(uint64_t val);
uint64_t safe_u64_to_u64(uint64_t val);
uint32_t safe_u64_to_u32(uint64_t val);
uint16_t safe_u64_to_u16(uint64_t val);
uint8_t safe_u64_to_u8(uint64_t val);

size_t safe_u32_to_size_t(uint32_t val);
uint64_t safe_u32_to_u64(uint32_t val);
uint32_t safe_u32_to_u32(uint32_t val);
uint16_t safe_u32_to_u16(uint32_t val);
uint8_t safe_u32_to_u8(uint32_t val);

size_t safe_u16_to_size_t(uint16_t val);
uint64_t safe_u16_to_u64(uint16_t val);
uint32_t safe_u16_to_u32(uint16_t val);
uint16_t safe_u16_to_u16(uint16_t val);
uint8_t safe_u16_to_u8(uint16_t val);

size_t safe_u8_to_size_t(uint8_t val);
uint64_t safe_u8_to_u64(uint8_t val);
uint32_t safe_u8_to_u32(uint8_t val);
uint16_t safe_u8_to_u16(uint8_t val);
uint8_t safe_u8_to_u8(uint8_t val);
