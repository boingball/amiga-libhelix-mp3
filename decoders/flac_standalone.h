#ifndef DECODERS_FLAC_STANDALONE_H
#define DECODERS_FLAC_STANDALONE_H

/*
 * Forced-include shim for the standalone Amiga FLAC decoder build.
 *
 * The decoder module is linked with -nostartfiles and must not pull newlib's
 * assert/abort/raise/dummy objects.  Define NDEBUG before assert.h is parsed
 * and then keep assert as a local no-op for the third-party foxen-flac source.
 */
#ifndef NDEBUG
#define NDEBUG 1
#endif

#include "flac_alloc.h"
#include <assert.h>

#ifdef assert
#undef assert
#endif
#define assert(x) ((void)0)

#endif /* DECODERS_FLAC_STANDALONE_H */
