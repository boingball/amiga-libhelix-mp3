# Huffman table-walk fast-path investigation

Date: 2026-06-17
Branch: `codex/huffman-tablewalk-fastpath`

## Decoder entry points used by `fast030`

`make -f Makefile.amiga fast030` builds `amiga_mp3dec.c`, `mp3dec.c`,
`mp3tabs.c`, and `real/*.c` with `-m68030 -O3 -fomit-frame-pointer` plus
`AMIGA_M68K_ASM_HUFFMAN`.  The frame core calls `DecodeHuffman()` in
`real/huffman.c`; its big-values regions call either
`DecodeHuffmanPairs_C_REFERENCE()` or, when `--exp-huff`/quality-0 enables the
experimental gate, `DecodeHuffmanPairs_TEST_ACTIVE()`.  On `fast030` the test
active path is the same C table walk with only the 16-bit refill primitive
changed to the inline m68k `move.l` helper.  Count1 still uses the C-only
`DecodeHuffmanQuads()` table walk.

## Static table-walk cost model

The pair decoder has four table classes:

| Class | Tables | Walk shape | Per-symbol branch/access shape |
| --- | --- | --- | --- |
| `noBits` | 0 | zero-fill only | no Huffman walk |
| `oneShot` | 1, 2, 3, 5, 6 | one metadata word, direct leaf lookup | refill branch; inner-loop branch; one table leaf access; two sign branches |
| `loopNoLinbits` | 7-13, 15 | root or subtable metadata per level | refill branch; inner-loop branch; one metadata access per level; one leaf/jump access per level; jump/leaf branch; two sign branches |
| `loopLinbits` | 16-31 except invalid gaps | same as loop tables, plus escape handling | no-linbits branches plus x/y escape branches, refill-for-linbits loops, and sign branches |

For the existing loop table walk, a non-escaped one-level symbol performs one
metadata read (`tCurr[0]`) and one indexed table read.  A two-level symbol repeats
that sequence after a jump, so it performs two metadata reads, two indexed table
reads, and one extra jump/leaf branch before sign handling.  Refill calls happen
only when the local cache drops below the inner-loop minimum; with the current
16-bit refill strategy this is roughly once per 16 consumed Huffman/sign bits,
plus special byte refills for linbits escapes.

## Candidate optimization considered

The most tempting small fast path is to flatten the root level of common
`loopNoLinbits` tables by caching each table's root `maxBits` and first subtable
base.  That would remove one `tCurr[0]` metadata load and the jump/leaf branch
for codes that always resolve at the root.  It does not affect sign bits,
linbits, table selection, or error recovery in principle.

However, the current compressed tables encode both leaves and jumps in the same
indexed word stream, and the correct consumed-bit count depends on the exact
metadata word at each visited level.  Adding an alternate flattened root table
would require generated side data and broad equivalence tests against valid MP3
frames before it should be enabled in the runtime decoder.  I therefore did not
change runtime Huffman semantics in this patch.

## Safe change made

No runtime fast path was introduced, and no decoder source was changed.  The
existing C path remains the reference and the runtime path.  This follows the
correctness rule for this investigation: without generated side data and broader
valid-frame equivalence coverage, the safe result is analysis only rather than a
semantic Huffman decoder change.

## Expected 68030 impact

Because no runtime optimization was enabled, expected 68030 decode impact is
0%.  The analysis points to a future low-risk target: generated root-level
flattening for `loopNoLinbits` tables, validated by comparing symbols,
consumed-bit counts, and error returns against `DecodeHuffmanPairs_C_REFERENCE()`.

## Required comparison policy

Any future fast path should be accepted only if reference and candidate paths
match for:

* decoded symbol values including sign-bit application;
* consumed-bit counts;
* error returns;
* linbits escapes;
* truncated/malformed payloads;
* deterministic random bitstreams;
* full-frame checksum output on representative MP3 files.

The existing `--selftest-huffman` harness already compares the C reference and
active pair path on deterministic pseudo-random bitstreams, including invalid
and truncated inputs.  Since no candidate path is enabled here, the fast-path hit
rate is 0 of tested symbols (0%).
