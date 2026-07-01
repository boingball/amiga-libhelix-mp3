/*
 * ogg_entry.c - OGG/Vorbis decoder module entry point.
 *
 * This file MUST be compiled and linked FIRST so that DecoderModuleEntry
 * lands at BADDR(seg)+4 — the first callable LONG in the code hunk.
 * Compile order: ogg_entry.c ogg_alloc.c ogg_module.c <tremor srcs> <libogg srcs>
 */

#include "decoder_module.h"

extern struct DecoderOps gOggOps;

/*
 * Keep the LoadSeg entry path as small as possible, matching aac_entry.c:
 * OggOpen() initialises the module allocator before the first allocation,
 * so nothing needs to happen here beyond returning the vtable.
 */
__attribute__((section(".text")))
struct DecoderOps *DecoderModuleEntry(void)
{
    return &gOggOps;
}
