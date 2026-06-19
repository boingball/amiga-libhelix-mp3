# MPEGA-style fused synthesis multiply-count proof

This note checks the 44.1 kHz input to 11.025 kHz output case, i.e. the
stride-4 / `freq_div == 4` fast-lowrate path.  Counts are per granule, per
channel.  Stereo doubles both sides, so the ratio is unchanged.

## Current MiniAMP3 stride-4 path

The opt-in stride-4 path runs `FDCT32Quarter()` once per synthesis block and
then emits eight PCM samples through the compact polyphase window.

* `FDCT32Quarter()` executes 20 `MULSHIFT32`/`muls.l` sites per block: eight in
  the symmetric first pass, eight in the retained radix-8 group, and four
  `COS4_0` butterflies in the final radix-2 stage.
* `PolyphaseMonoFastCompactSample()` / the per-channel half of
  `PolyphaseStereoFastCompactSample()` uses the `FAST_MC*` compact dewindow
  macros.  For stride 4 it emits eight samples per block, each with a 16-tap
  dot product, for 128 `PolyphaseMulShift26()` multiplies per block per
  channel.
* There are 18 synthesis blocks per granule.

Therefore the current stride-4 path costs:

```text
(20 FDCT32Quarter muls + 8 samples * 16 window muls) * 18 blocks
= (20 + 128) * 18
= 2,664 muls.l per granule per channel
```

## MPEGA-equivalent fused `freq_div == 4` path

The MPEGA-style low-rate structure runs only the half-depth butterfly for
`freq_div == 4`, then windows the lower-rate outputs with a quality-0
`w_width == 4` dewindow.

* The `sub_half_dct`-equivalent butterfly has 12 cosine-node multiplies for the
  retained half-depth transform.
* `window_band` with `w_width == 4` performs four multiplies per emitted
  sample.  At stride 4 the path emits the same eight samples per block, so the
  window costs 32 multiplies per block per channel.
* There are still 18 synthesis blocks per granule.

Projected fused cost:

```text
(12 half-depth butterfly muls + 8 samples * 4 window muls) * 18 blocks
= (12 + 32) * 18
= 792 muls.l per granule per channel
```

## Ratio and decision

```text
2,664 / 792 = 3.36x fewer synthesis-stage multiplies
```

The projected reduction is well above the requested ~1.5x threshold, so the
compile-gated fused backend is worth implementing as an experimental opt-in
path.

## Implemented Stage 1 count

The Stage 1 implementation keeps only the `freq_div == 4` fused path live.  Its
compiled `FusedSubHalfDCT()` body has 12 `FusedMulCosQ29()` butterfly-node
multiplies, matching the projection.  Its `FusedWindowBand4Sample()` body uses
four `FusedPolyMulShift26()` dewindow multiplies for each of the same eight
emitted stride-4 samples per synthesis block.

```text
(12 FusedSubHalfDCT muls + 8 samples * 4 fused window muls) * 18 blocks
= (12 + 32) * 18
= 792 muls.l-equivalent multiplies per granule per channel
```

The implemented Stage 1 count therefore matches the projected 792 multiplies per
granule per channel and preserves the projected 2,664 / 792 = 3.36x reduction.
Stride-2 remains deliberately disabled in the fused dispatcher until the Stage 2
butterfly depth is implemented and reviewed.
