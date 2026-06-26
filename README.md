# Amiga libhelix MP3/AAC/FLAC Player

Classic AmigaOS m68k audio player based on the Helix fixed-point decoders, with local MP3 playback, AAC-LC ADTS support, FLAC decoder modules, ReAction/GadTools front-ends, m68k optimisations, and experimental HTTP MP3 internet radio with ICY metadata.

This started as an Amiga port of the Helix MP3 decoder and has grown into a modular audio player for classic Amiga systems and emulators.

## Current features

- Local MP3 playback using the Helix fixed-point MP3 decoder
- m68k optimised MP3 decode path
- AAC-LC ADTS playback via external `aac.decoder`
- Optional AAC m68k asm helpers with `AACASM=1`
- FLAC playback via external `flac.decoder`
- Modular decoder loading
- ReAction GUI front-end
- GadTools GUI front-end
- CLI playback mode
- HTTP MP3 internet radio support with `RADIO=1`
- ICY metadata parsing
- Live title/artist updates from radio streams
- Radio station, genre, bitrate and content-type display
- Resilient radio buffering and reconnect handling
- Paula playback output
- Fast low-rate playback options for slower classic systems

## Screenshots

Add screenshots here when ready.

```text
docs/screenshots/
```

Suggested screenshots:

- ReAction GUI playing a local MP3
- ReAction GUI playing HTTP radio with ICY metadata
- GadTools GUI
- CLI playback output

## Supported formats

| Format | Status | Notes |
|---|---:|---|
| MP3 | Working | Main supported format. Helix fixed-point decoder with m68k optimisation. |
| AAC-LC ADTS | Working | External decoder module. ADTS `.aac` streams/files only. |
| FLAC | Working | External decoder module. Performance depends heavily on CPU. |
| HTTP MP3 radio | Working | Plain `http://` MP3 streams. ICY metadata supported. |
| HTTPS radio | Not supported | Use plain HTTP streams for now. |
| HLS / M3U8 | Not supported | Out of scope currently. |
| AAC+ / HE-AAC radio | Not supported yet | AAC-LC ADTS support exists, but AAC radio is not wired as a supported target yet. |

## Internet radio

The player can stream direct HTTP MP3 radio streams when built with `RADIO=1`.

Known working test stream:

```text
http://ice1.somafm.com/groovesalad-128-mp3
```

Expected metadata from this stream:

```text
Station: Groove Salad
Genre: Ambient Chill
Bitrate: 128 kbps
Content-Type: audio/mpeg
Title/Artist: updates automatically from ICY StreamTitle
Track: Live
```

Radio support currently expects direct plain HTTP MP3 streams. It does not currently support HTTPS, HLS, playlist parsing, or AAC+ radio.

## Build requirements

- Bebbo m68k Amiga GCC toolchain
- `m68k-amigaos-gcc`
- `m68k-amigaos-nm`
- GNU Make
- Git with submodule support

Check the toolchain:

```sh
which m68k-amigaos-gcc
which m68k-amigaos-nm
m68k-amigaos-gcc --version | head -3
```

## Full build instructions

Full AmigaOS build instructions are in:

```text
docs/building-amiga.md
```

That document covers:

- clean repo sync
- submodule sync
- FLAC decoder build
- AAC decoder build
- AAC asm build with `AACASM=1`
- radio build with `RADIO=1`
- decoder entrypoint checks
- copying files to Amiga/WinUAE
- runtime tests
- Git hygiene

## Quick build

From a clean checkout:

```sh
cd ~/Amiga-Programs/libhelix-mp3

git fetch origin --prune
git checkout master
git reset --hard origin/master
git clean -fd

git submodule sync --recursive
git submodule update --init --recursive
git submodule foreach --recursive 'git reset --hard && git clean -fd'

make -C decoders clean || true
find . -name "*.o" -delete
rm -f amiga_mp3dec.fastexp minimp3r
rm -f decoders/*.decoder decoders/*.decoder.map

make -C decoders flac
make -C decoders aac AACASM=1
make -f Makefile.amiga fast030 RADIO=1
```

Verify decoder module entrypoints:

```sh
m68k-amigaos-nm -n decoders/aac.decoder | head -10
m68k-amigaos-nm -n decoders/flac.decoder | head -10
```

Both should start with:

```text
00000000 T _DecoderModuleEntry
```

If anything appears before `_DecoderModuleEntry`, the module may crash when loaded.

## Runtime layout

Keep the player and decoder modules together.

Typical Amiga-side layout:

```text
libhelix-mp3/
  minimp3r
  amiga_mp3dec.fastexp
  decoders/
    aac.decoder
    flac.decoder
```

Depending on the build target/front-end, the executable may be:

```text
amiga_mp3dec.fastexp
minimp3r
```

## Runtime tests

Local MP3:

```text
amiga_mp3dec.fastexp --play test.mp3
```

AAC:

```text
amiga_mp3dec.fastexp --play test.aac
```

FLAC:

```text
amiga_mp3dec.fastexp --play test.flac
```

HTTP MP3 radio:

```text
amiga_mp3dec.fastexp --play "http://ice1.somafm.com/groovesalad-128-mp3"
```

## Decoder modules

External decoder modules must export `DecoderModuleEntry` as the first real text symbol.

Required check:

```sh
m68k-amigaos-nm -n decoders/aac.decoder | head -10
m68k-amigaos-nm -n decoders/flac.decoder | head -10
```

Expected:

```text
00000000 T _DecoderModuleEntry
```

This is important because the Amiga module loader expects to enter the decoder module at the correct offset. If compiler helper code or library code appears before `DecoderModuleEntry`, the module can jump into the wrong code and crash.

## AAC notes

AAC support currently targets AAC-LC ADTS files.

The AAC decoder uses the `decoders/esp8266audio` submodule, with the AAC source under:

```text
decoders/esp8266audio/src/libhelix-aac
```

The expected local symlink is:

```text
decoders/aac -> esp8266audio/src/libhelix-aac
```

Build AAC normally:

```sh
make -C decoders aac
```

Build AAC with optional m68k asm helpers:

```sh
make -C decoders aac AACASM=1
```

`AACASM=1` enables optional m68k helper paths for:

```text
AMIGA_M68K_ASM_AAC_HUFFMAN
AMIGA_M68K_ASM_AAC_DEQUANT
AMIGA_M68K_ASM_AAC_STEREO
AMIGA_M68K_ASM_AAC_IMDCT
```

The plain C fallback remains available by building without `AACASM=1`.

## FLAC notes

FLAC support is provided by `flac.decoder`.

Build:

```sh
make -C decoders flac
```

FLAC is heavier than MP3 and performance depends on CPU, file complexity, output rate and playback settings.

## Radio notes

Build with radio support:

```sh
make -f Makefile.amiga fast030 RADIO=1
```

Radio support uses Amiga `bsdsocket.library` at runtime.

Important notes:

- Use direct plain HTTP MP3 stream URLs.
- HTTPS is not currently supported.
- HLS/M3U8 is not currently supported.
- ICY metadata is stripped from the audio stream before MP3 decode.
- ICY `StreamTitle` updates the GUI title/artist fields.
- Radio streams are live, so duration is displayed as Live.

## Known good radio stream

```text
http://ice1.somafm.com/groovesalad-128-mp3
```

Useful header check on the host:

```sh
curl -I http://ice1.somafm.com/groovesalad-128-mp3
```

Expected headers include:

```text
Content-Type: audio/mpeg
icy-br: 128
icy-genre: Ambient Chill
icy-name: Groove Salad: a nicely chilled plate of ambient beats and grooves. [SomaFM]
icy-url: http://somafm.com
```

## Development notes

Do not commit generated files:

```text
*.o
*.decoder
*.decoder.map
*.fastexp
minimp3r
test audio files
*:Zone.Identifier
```

Before committing:

```sh
git status --short
```

Recommended final test checklist before pushing decoder/player changes:

```text
MP3 local playback
AAC local playback
AAC TNS-heavy file
FLAC local playback
HTTP MP3 radio playback
ICY title/artist metadata updates
Stop on radio stream
Menus after stopping radio
Restart radio after stopping
DecoderModuleEntry still at offset 0
```

## Project status

This is an active classic Amiga development project.

Current focus areas:

- ReAction GUI polish
- GadTools GUI parity
- FLAC performance improvements
- AAC performance improvements
- radio preset/station handling
- artwork/station logo support
- further m68k optimisation

## Credits

This project builds on fixed-point decoder work from the Helix family of decoders and related open-source audio decoder code.

Amiga port, decoder module integration, GUI work, radio streaming and m68k optimisation work by boingball.

## Licence

See the repository licence and the licences of included/submodule decoder sources.
