Decoders for MiniAMP3

Flac - https://github.com/astoeckel/libfoxenflac - 	
Re-license as LGPLv2.1 or later

WAV

AAC - https://github.com/earlephilhower/ESP8266Audio/blob/master/src/libhelix-aac/aacdec.c
LibHelix AAC Decoder
Source: place libhelix-aac files in decoders/aac/ (see Makefile for instructions)

ALAC  - https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/alac.c
FFMPEG ALAC

OGG - Tremor (fixed-point Vorbis) + libogg — IMPLEMENTED, see ogg_module.c
  https://github.com/pschatzmann/arduino-libvorbis-tremor (upstream: https://gitlab.xiph.org/xiph/tremor)
  https://github.com/xiph/ogg
BSD-style license, same family as libogg and libvorbis, see LICENSE.txt for information.
Source: place Tremor in decoders/tremor/ and libogg in decoders/libogg/ (see Makefile
for submodule instructions). Chosen over https://github.com/edubart/minivorbis because
Tremor is integer/fixed-point only, matching the FLAC (libfoxenflac) and AAC
(libhelix-aac) decoders already in this project — no FPU dependency.

OPUS - https://github.com/xiph/opus/blob/main/src/opus_decoder.c
Also vendored via the ESP8266Audio submodule under src/libopus. Considered instead of
Tremor for OGG support and rejected for now: it's floating-point by default, and even
its --enable-fixed-point build is the full modern SILK+CELT reference codebase — a much
bigger port than this project's other decoders, with real-time decode on 68000/68020
uncertain.

M4A - Too much?

WMA - https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/wmaprodec.c

WEBM - https://github.com/SAV1-org/SAV1

