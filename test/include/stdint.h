/* stdint.h */
#ifndef CLAUDEC99_STDINT_H
#define CLAUDEC99_STDINT_H

/* exact-width signed integer types */
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;

/* exact-width unsigned integer types */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

/* pointer-size integer types */
typedef long intptr_t;
typedef unsigned long uintptr_t;

/* minimum-width signed integer types */
typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

/* minimum-width unsigned integer types */
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

/* fast signed integer types - simple LP64 choices */
typedef int int_fast8_t;
typedef int int_fast16_t;
typedef int int_fast32_t;
typedef long int_fast64_t;

/* fast unsigned integer types - simple LP64 choices */
typedef unsigned int uint_fast8_t;
typedef unsigned int uint_fast16_t;
typedef unsigned int uint_fast32_t;
typedef unsigned long uint_fast64_t;

/* greatest-width integer types */
typedef long intmax_t;
typedef unsigned long uintmax_t;

#endif
