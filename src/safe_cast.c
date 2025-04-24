#include "safe_cast.h"

#include <assert.h>

#define create_safe_cast_fn(NAME, FROM, TO) TO NAME(FROM val) { \
    assert(val < (FROM)~(TO)0); \
    return (TO)val; \
}

create_safe_cast_fn(safe_size_t_to_size_t, size_t, size_t)
create_safe_cast_fn(safe_size_t_to_u64, size_t, uint64_t)
create_safe_cast_fn(safe_size_t_to_u32, size_t, uint32_t)
create_safe_cast_fn(safe_size_t_to_u16, size_t, uint16_t)
create_safe_cast_fn(safe_size_t_to_u8, size_t, uint8_t)

create_safe_cast_fn(safe_u64_to_size_t, uint64_t, size_t)
create_safe_cast_fn(safe_u64_to_u64, uint64_t, uint64_t)
create_safe_cast_fn(safe_u64_to_u32, uint64_t, uint32_t)
create_safe_cast_fn(safe_u64_to_u16, uint64_t, uint16_t)
create_safe_cast_fn(safe_u64_to_u8, uint64_t, uint8_t)

create_safe_cast_fn(safe_u32_to_size_t, uint32_t, size_t)
create_safe_cast_fn(safe_u32_to_u64, uint32_t, uint64_t)
create_safe_cast_fn(safe_u32_to_u32, uint32_t, uint32_t)
create_safe_cast_fn(safe_u32_to_u16, uint32_t, uint16_t)
create_safe_cast_fn(safe_u32_to_u8, uint32_t, uint8_t)

create_safe_cast_fn(safe_u16_to_size_t, uint16_t, size_t)
create_safe_cast_fn(safe_u16_to_u64, uint16_t, uint64_t)
create_safe_cast_fn(safe_u16_to_u32, uint16_t, uint32_t)
create_safe_cast_fn(safe_u16_to_u16, uint16_t, uint16_t)
create_safe_cast_fn(safe_u16_to_u8, uint16_t, uint8_t)

create_safe_cast_fn(safe_u8_to_size_t, uint8_t, size_t)
create_safe_cast_fn(safe_u8_to_u64, uint8_t, uint64_t)
create_safe_cast_fn(safe_u8_to_u32, uint8_t, uint32_t)
create_safe_cast_fn(safe_u8_to_u16, uint8_t, uint16_t)
create_safe_cast_fn(safe_u8_to_u8, uint8_t, uint8_t)
