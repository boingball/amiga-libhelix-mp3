# Amiga fast-lowrate loop-overhead pass notes

## Target command

```sh
amiga_mp3dec_fastpoly --decode-only --bench --checksum --fast-lowrate --rate 11025 gs-16b-1c-44100hz.mp3
```

The pass is deliberately output-neutral: it changes only loop control, phase
selection, pointer movement, and invariant setup in the fast-lowrate synthesis
path.  The convolution helpers, coefficient tables, clipping, and emitted sample
order are unchanged.

## Before profile supplied for this pass

Build/profile: `AMIGA_PROFILE_DECODE + AMIGA_FAST_POLYPHASE + --fast-lowrate --rate 11025`.

| Bucket | Before elapsed |
| --- | ---: |
| polyphase | ~9.34 s |
| subband/dct32 | ~5.64 s |
| imdct | ~5.14 s |
| huffman | ~2.10 s |
| dequant | ~1.96 s |

## Changes made

- Fast-lowrate stride 4 and stride 5 now use precomputed phase-to-sample tables.
  This removes the per-output-block sample loop's phase test/modulo-style wrap
  from the common `--rate 11025` and `--rate 8287` paths.
- The fast-lowrate emitters now walk sample tables and output pointers directly,
  rather than recomputing `produced` indexes for each emitted mono/stereo sample.
- Subband now splits fast-lowrate and full-rate loops once per granule, hoists
  `stride`, `phase`, `fastLowrateOutputSamps`, and `vindex` into locals, and
  writes them back after the block loop.
- The DCT/polyphase block loop now computes the odd-block flag and active vbuf
  base once per block instead of repeating those expressions in each call site.

## Checksum/benchmark log

The required MP3 fixture (`gs-16b-1c-44100hz.mp3`) was not present in this
workspace, so the exact before/after checksum lines must be filled in on the
Amiga benchmark machine that has the fixture.

| Rate | Before checksum | After checksum | Before elapsed | After elapsed |
| --- | --- | --- | ---: | ---: |
| 11025 | _record from pre-pass binary_ | _must match before_ | polyphase ~9.34 s | _record on target_ |
| 8287 | _record from pre-pass binary_ | _must match before_ | _record on target_ | _record on target_ |

Suggested commands:

```sh
amiga_mp3dec_fastpoly.before --decode-only --bench --checksum --fast-lowrate --rate 11025 gs-16b-1c-44100hz.mp3
amiga_mp3dec_fastpoly.after  --decode-only --bench --checksum --fast-lowrate --rate 11025 gs-16b-1c-44100hz.mp3
amiga_mp3dec_fastpoly.before --decode-only --bench --checksum --fast-lowrate --rate 8287  gs-16b-1c-44100hz.mp3
amiga_mp3dec_fastpoly.after  --decode-only --bench --checksum --fast-lowrate --rate 8287  gs-16b-1c-44100hz.mp3
```

## `real/*.c` memset/memcpy audit

- `real/*.c` has no per-frame `memset`, `memcpy`, or `memmove` in the decode hot
  path.  The only local clearing helper is `ClearBuffer()` in `real/buffers.c`,
  used during decoder allocation/initialization, not per frame or granule.
- The per-frame bit reservoir movement lives in `mp3dec.c`: it preserves prior
  main-data bytes with `memmove()` only when `mainDataBegin > 0`, then appends
  new slot bytes with `memcpy()`.  That copy is format-required reservoir state
  and is not safe to remove without a ring-buffer rewrite.
- The Amiga front end uses `memmove()`/`memcpy()` for input-buffer compaction,
  rate-conversion scratch movement, and output formatting.  These are outside
  `real/*.c` and outside the requested decode-core loop pass; no safe reduction
  was made here.
