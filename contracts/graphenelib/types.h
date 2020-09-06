#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @defgroup types Builtin Types
 *  @ingroup contractdev
 *  @brief Specifies typedefs and aliases
 *
 *  @{
 */

#define GRAPHENE_DB_MAX_INSTANCE_ID  (uint64_t(-1)>>16)

typedef uint64_t table_name;
typedef uint32_t time;
typedef uint64_t scope_name;
typedef uint64_t action_name;

typedef uint16_t weight_type;

/* macro to align/overalign a type to ensure calls to intrinsics with pointers/references are properly aligned */
#define ALIGNED(X) __attribute__ ((aligned (16))) X

struct public_key {
   char data[33];
};

struct signature {
   uint8_t data[65];
};

struct ALIGNED(checksum256) {
   uint8_t hash[32];
};

struct ALIGNED(checksum160) {
   uint8_t hash[20];
};

struct ALIGNED(checksum512) {
   uint8_t hash[64];
};

typedef struct checksum160      block_id_type;

#ifdef __cplusplus
} /// extern "C"
#endif
/// @}
