# Amiga 68030 DCT/IMDCT `muls.l` reduction investigation

This note records the exact-path investigation for reducing signed 32-bit
multiply use in the Helix MP3 DCT/IMDCT hot paths used by the Amiga `fast030`
build. No runtime transform behavior is changed by this investigation because
no safe, bit-identical multiply reduction was found.

## Fast030 transform entry points

The `fast030` make target enables `AMIGA_M68K_ASM_FDCT32`,
`AMIGA_M68K_ASM_FDCT32_UNROLL`, and `AMIGA_M68K_ASM_IMDCT`. With those macros,
the hot transform paths are:

* `FDCT32()` in `real/dct32.c`, which selects the Amiga/m68k assembly first
  pass, second pass, and output shuffle when `FDCT32_HAS_AMIGA_M68K_ASM` is
  true. `FDCT32_C_REFERENCE()` remains the reference path.
* `FDCT32Half()` and `FDCT32Quarter()` may be used by low-rate synthesis before
  polyphase, but they are existing stride-specific optimisations and were not
  altered.
* `IMDCT36_TEST_ACTIVE()` in `real/imdct.c`, which selects
  `IMDCT36_AMIGA_M68K_ASM()` only for the common long-window case
  `btCurr == 0 && btPrev == 0`; all other block-type combinations fall back to
  `IMDCT36_C_REFERENCE()`.

## Current multiply counts

### FDCT32 full path

The documented full 32-point DCT count remains 80 high-word multiplies per call:

* first radix-4 pass: 8 groups * 4 `muls.l` = 32;
* second pass: 4 radix-8 groups * 12 `muls.l` = 48.

The Amiga assembly mirrors this count:

* `FDCT32_M68K_FIRST_BUTTERFLY` contains 4 `muls.l` instructions and is emitted
  8 times;
* `FDCT32_M68K_SECOND_GROUP` contains 12 `muls.l` instructions and is emitted
  4 times in the unrolled `fast030` build.

### IMDCT36 common long-window path

The documented common long-window count remains 47 high-word multiplies per
call:

* two 9-point IDCTs: 2 * 10 = 20;
* last IMDCT stage: 9;
* fused long-window/overlap stage: 18.

The Amiga assembly mirrors this count:

* `idct9_amiga_m68k_asm()` performs 10 high-word multiplies and is called twice;
* `IMDCT36_AMIGA_M68K_LONG_WINDOW()` performs three `muls.l` instructions in
  each of nine iterations.

The non-common IMDCT36 reference path still has the documented 65 multiply
count in the general case and is not selected by the Amiga assembly dispatcher.

## Candidate expressions reviewed

### FDCT32 final radix-2 `COS4_0` multiplies

The second-pass final radix-2 stage repeatedly multiplies pairwise differences
by `COS4_0`. Although the same constant appears repeatedly, each multiply input
is different. Factoring adjacent terms would require assuming
`hi32(k*a) + hi32(k*b) == hi32(k*(a+b))`, which is not generally true because
`MULSHIFT32` keeps the signed high 32 bits of a 32x32 product and truncates each
product independently. This would change rounding/truncation behavior.

### FDCT32 first and second radix butterfly products

The butterfly shapes are repeated, but the coefficient stream and input
differences differ in each group. No same-input/same-constant product is
recomputed. Sign-symmetric products are already represented as signed constants
in the table where applicable; replacing them with a shared positive multiply
and a negate would not reduce the multiply count.

### IMDCT `idct9` shared inputs with different constants

The 9-point IDCT computes pairs such as `a5*c9_1` and `a5*c9_2`, plus
`a9*c9_3` and `a9*c9_4`. These reuse an input but not a coefficient. Deriving
one product from another would either approximate fixed-point constants or move
truncation to a different point, so it is not bit-identical.

### IMDCT long-window repeated `t = s - d`

The long-window assembly computes `t = s - d` before each of two window
multiplies. Sharing the register value would add register pressure or stores,
but it would not remove a `muls.l`: the two products use different consecutive
`fastWin36` constants. The current recomputation is two cheap register
operations and avoids keeping another live value across a multiply and FASTABS
sequence.

### Shift/add replacements for constants

The hot constants are Q31/Q30/Q29 fixed-point cosine/window constants with dense
bit patterns. None are zero, one, negative one, powers of two, or sparse enough
to replace exactly with a short add/sub/shift sequence that preserves the high
32-bit signed product semantics. A shift/add approximation was rejected because
precision and coefficients must not change.

## Result

No safe multiply reduction was implemented. The runtime path remains unchanged,
so no selectable optimized transform or optimized-vs-reference selftest was
added. Existing reference/active selftests remain the correct comparison tools
for the current assembly paths.

Multiply counts before and after this investigation are identical:

| Path | Before | After | Delta |
| --- | ---: | ---: | ---: |
| FDCT32 full | 80 | 80 | 0 |
| IMDCT36 common long window | 47 | 47 | 0 |
| IMDCT36 general reference | 65 | 65 | 0 |

Additional runtime add/sub/load/store operations: none.

Register-pressure implications: none, because no runtime code was changed. The
reviewed sharing candidates would increase pressure in the already register-
starved 68030 assembly regions unless they also removed a multiply, which they
do not.

Expected 68030 benefit: none from this patch. The investigation avoids a
speculative change that could silently alter high-word multiply truncation,
rounding, scaling, or output ordering.

Bit-identical output: yes by construction, because transform runtime code is
unchanged.
