#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

/*
 * Hand-written stand-in for libogg's autotools/CMake-generated
 * include/ogg/config_types.h (normally produced from config_types.h.in by
 * ./configure or cmake during a native libogg build, which this project's
 * standalone decoder-module Makefile does not run).
 *
 * m68k-amigaos-gcc provides a standard <stdint.h>, so these are just the
 * fixed-width aliases libogg expects — same definitions the real generated
 * header produces on any stdint.h-capable platform.
 */

#include <stdint.h>

typedef int16_t  ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t  ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t  ogg_int64_t;
typedef uint64_t ogg_uint64_t;

#endif  /* __CONFIG_TYPES_H__ */
