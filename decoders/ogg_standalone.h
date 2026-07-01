#ifndef DECODERS_OGG_STANDALONE_H
#define DECODERS_OGG_STANDALONE_H

/*
 * Forced-include shim for the standalone Amiga OGG/Vorbis (Tremor) decoder
 * build.
 *
 * The decoder module is linked with -nostartfiles and must not pull newlib's
 * assert/abort/raise/dummy objects.  Define NDEBUG before assert.h is parsed
 * and then keep assert as a local no-op for the third-party libogg/Tremor
 * source, matching flac_standalone.h / aac_standalone.h.
 */
#ifndef NDEBUG
#define NDEBUG 1
#endif

#include "ogg_alloc.h"
#include <assert.h>

#ifdef assert
#undef assert
#endif
#define assert(x) ((void)0)

#endif /* DECODERS_OGG_STANDALONE_H */
