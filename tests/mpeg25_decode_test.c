/*
 * MPEG-2.5 Layer III decode test.
 *
 * Helix has always carried the MPEG-2.5 tables - samplerateTab, bitrateTab and
 * sfBandTable each have a third row for 8/11.025/12 kHz, and
 * UnpackFrameHeader() maps version index 0 to MPEG25 - but pub/mp3common.h
 * shipped with the 12-bit syncword, which only matches MPEG-1 and MPEG-2
 * frames. An MPEG-2.5 frame's second header byte is 0xe2/0xe3, so
 * MP3FindSyncWord() never found one and those files decoded to nothing at all:
 * no error, no samples, silence. Those are exactly the rates a low-bitrate
 * encode for Paula lands on, which is how it stayed unnoticed.
 *
 * This holds the whole path down: the syncword finds a 2.5 frame, the header
 * decoder reports its rate and its 576-sample granule for all three 2.5 rates
 * (and still reports 1152 for MPEG-1), and a real 2.5 stream decodes to the
 * samples it should.
 */
#include <stdio.h>
#include <string.h>

#include "mp3dec.h"

/* Five frames of 11.025 kHz mono Layer III at 8 kbit/s - 0.15 s of a 440 Hz
 * tone - and nothing else: no Xing/LAME frame and no ID3 tag, so the buffer is
 * exactly the audio frames. Produced with:
 *
 *   ffmpeg -f lavfi -i "sine=frequency=440:sample_rate=11025:duration=0.15" \
 *          -c:a libmp3lame -b:a 8k -ar 11025 -ac 1 \
 *          -write_xing 0 -id3v2_version 0 -f mp3 fixture.mp3
 */
static const unsigned char mpeg25_stream[] = {
	0xff, 0xe3, 0x10, 0xc4, 0x00, 0x08, 0x78, 0x06, 0xdd, 0xbf, 0x41, 0x08,
	0x02, 0xff, 0xf2, 0x1c, 0x18, 0x00, 0x3f, 0x00, 0x0c, 0x1f, 0x07, 0xc1,
	0xf9, 0x40, 0x40, 0x10, 0x71, 0xc8, 0x20, 0x08, 0x39, 0xe2, 0x70, 0xf8,
	0x7f, 0xf2, 0xe1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xa7, 0xdd, 0xd2, 0x22,
	0x1f, 0x78, 0x8a, 0x21, 0xff, 0xe3, 0x12, 0xc4, 0x05, 0x09, 0x68, 0xaa,
	0xa8, 0x01, 0x8a, 0xc8, 0x00, 0xf1, 0x12, 0x04, 0xbf, 0x01, 0x03, 0xaa,
	0x0e, 0x35, 0xe9, 0x0a, 0x6e, 0xe5, 0xbe, 0xc6, 0x45, 0x0c, 0x5a, 0x55,
	0x1a, 0x95, 0x63, 0xff, 0xff, 0xff, 0xff, 0x8d, 0x5d, 0x03, 0x5f, 0x12,
	0x9d, 0x55, 0x30, 0x76, 0x02, 0x23, 0x5b, 0x92, 0x98, 0xff, 0xe3, 0x10,
	0xc4, 0x07, 0x09, 0x98, 0x96, 0x40, 0x01, 0x80, 0xf0, 0x00, 0x32, 0xb8,
	0x08, 0xa0, 0x60, 0x8e, 0x99, 0x4e, 0x49, 0xf9, 0x8a, 0x98, 0x76, 0x98,
	0x3f, 0x88, 0x51, 0x9f, 0xc4, 0x18, 0x18, 0xe1, 0x84, 0x05, 0xd9, 0x57,
	0xfe, 0xa1, 0x3c, 0xfd, 0x40, 0xea, 0xf1, 0x20, 0x47, 0xf2, 0x81, 0x36,
	0x8c, 0xff, 0xe3, 0x10, 0xc4, 0x07, 0x0a, 0x58, 0xca, 0xa8, 0x01, 0x87,
	0xa0, 0x00, 0x12, 0x00, 0x19, 0x68, 0x06, 0x97, 0x06, 0xd0, 0x3e, 0x40,
	0x01, 0x98, 0x9d, 0xc4, 0x84, 0x08, 0x92, 0x05, 0x0b, 0xff, 0x91, 0x32,
	0x81, 0xa9, 0x6b, 0xff, 0x37, 0x04, 0x80, 0x7f, 0x8b, 0x18, 0x6f, 0xf3,
	0x55, 0xc5, 0x2c, 0x89, 0xad, 0xff, 0xe3, 0x10, 0xc4, 0x04, 0x09, 0x30,
	0x96, 0x80, 0x09, 0xc9, 0x30, 0x00, 0xb5, 0x50, 0xa1, 0xd8, 0xc7, 0xc9,
	0xab, 0x21, 0x14, 0xcc, 0x84, 0x32, 0x4b, 0x92, 0xd5, 0x50, 0xe9, 0x10,
	0x08, 0x49, 0x1c, 0xa2, 0x44, 0xa4, 0x44, 0x78, 0xa8, 0x6b, 0x22, 0xb3,
	0xb3, 0xb0, 0x69, 0x4c, 0x41, 0x4d, 0x45, 0x55, 0x55,
};

#define MPEG25_FRAMES        5
#define MPEG25_GRANULE       576
#define MPEG25_SAMPLES       (MPEG25_FRAMES * MPEG25_GRANULE)

/* Decoded by this decoder and cross-checked against ffmpeg's own decode of the
 * same 261 bytes: sample for sample, with no delay between them, worst
 * difference 71 and mean 4.0 against a mean amplitude of 1480 - that is
 * fixed-point rounding, not a different decode. These two totals pin that
 * result without carrying 2880 expected samples around. */
#define MPEG25_ABS_SUM       4260772L
#define MPEG25_HASH          0x2464e39aUL

static int failures;

static void check(int ok, const char *what, long got, long want)
{
	if (ok) {
		printf("ok: %s (%ld)\n", what, got);
		return;
	}
	printf("FAIL: %s: got %ld, expected %ld\n", what, got, want);
	failures++;
}

/* A bare frame header for each version, so the header decoder can be asked
 * about a rate without a whole stream behind it:
 *   MPEG-1   Layer III 44.1 kHz 128 kbit/s mono
 *   MPEG-2   Layer III 22.05 kHz 32 kbit/s mono
 *   MPEG-2.5 Layer III 11.025 kHz 32 kbit/s mono
 *   MPEG-2.5 Layer III 12 kHz 32 kbit/s mono
 *   MPEG-2.5 Layer III 8 kHz 32 kbit/s mono
 * Every one of these needs 32 bytes behind it for MP3GetNextFrameInfo() to
 * read, hence the padding. */
struct header_case {
	const char     *name;
	unsigned char   header[4];
	int             samprate;
	int             outputSamps;
};

static const struct header_case header_cases[] = {
	{ "MPEG-1 44100",   { 0xff, 0xfb, 0x90, 0xc0 }, 44100, 1152 },
	{ "MPEG-2 22050",   { 0xff, 0xf3, 0x40, 0xc0 }, 22050,  576 },
	{ "MPEG-2.5 11025", { 0xff, 0xe3, 0x40, 0xc0 }, 11025,  576 },
	{ "MPEG-2.5 12000", { 0xff, 0xe3, 0x44, 0xc0 }, 12000,  576 },
	{ "MPEG-2.5 8000",  { 0xff, 0xe3, 0x48, 0xc0 },  8000,  576 }
};

int main(void)
{
	HMP3Decoder     dec;
	MP3FrameInfo    info;
	unsigned char   probe[64];
	unsigned char  *readPtr;
	short           pcm[2 * 1152];
	short           out[MPEG25_SAMPLES];
	unsigned long   hash = 0;
	long            absSum = 0;
	int             i, bytesLeft, err, frames = 0, samples = 0, offset;

	dec = MP3InitDecoder();
	if (!dec) {
		printf("FAIL: MP3InitDecoder\n");
		return 1;
	}

	/* The syncword has to match an MPEG-2.5 frame, and has to still be found
	 * when it does not start at the first byte. */
	memset(probe, 0x5a, sizeof probe);
	memcpy(probe + 7, mpeg25_stream, sizeof probe - 7);
	offset = MP3FindSyncWord(probe, (int)sizeof probe);
	check(offset == 7, "MP3FindSyncWord finds an MPEG-2.5 frame", offset, 7);

	for (i = 0; i < (int)(sizeof header_cases / sizeof header_cases[0]); i++) {
		memset(probe, 0, sizeof probe);
		memcpy(probe, header_cases[i].header, 4);
		memset(&info, 0, sizeof info);
		err = MP3GetNextFrameInfo(dec, &info, probe);
		if (err != ERR_MP3_NONE) {
			printf("FAIL: %s: MP3GetNextFrameInfo returned %d\n",
			       header_cases[i].name, err);
			failures++;
			continue;
		}
		check(info.samprate == header_cases[i].samprate,
		      header_cases[i].name, info.samprate,
		      header_cases[i].samprate);
		check(info.outputSamps == header_cases[i].outputSamps,
		      "  granule samples", info.outputSamps,
		      header_cases[i].outputSamps);
	}

	/* And the stream decodes. */
	readPtr = (unsigned char *)mpeg25_stream;
	bytesLeft = (int)sizeof mpeg25_stream;
	while (bytesLeft > 0) {
		offset = MP3FindSyncWord(readPtr, bytesLeft);
		if (offset < 0)
			break;
		readPtr += offset;
		bytesLeft -= offset;
		err = MP3Decode(dec, &readPtr, &bytesLeft, pcm, 0);
		if (err == ERR_MP3_MAINDATA_UNDERFLOW)
			continue;
		if (err != ERR_MP3_NONE) {
			printf("FAIL: MP3Decode returned %d on frame %d\n",
			       err, frames);
			failures++;
			break;
		}
		MP3GetLastFrameInfo(dec, &info);
		if (frames == 0) {
			check(info.samprate == 11025, "stream sample rate",
			      info.samprate, 11025);
			check(info.nChans == 1, "stream channels", info.nChans, 1);
			check(info.outputSamps == MPEG25_GRANULE, "stream granule",
			      info.outputSamps, MPEG25_GRANULE);
		}
		for (i = 0; i < info.outputSamps && samples < MPEG25_SAMPLES; i++)
			out[samples++] = pcm[i];
		frames++;
	}
	MP3FreeDecoder(dec);

	check(frames == MPEG25_FRAMES, "frames decoded", frames, MPEG25_FRAMES);
	check(samples == MPEG25_SAMPLES, "samples decoded", samples,
	      MPEG25_SAMPLES);

	for (i = 0; i < samples; i++) {
		long v = out[i];
		absSum += v < 0 ? -v : v;
		hash = ((hash * 33UL) ^ ((unsigned long)out[i] & 0xffffUL))
		     & 0xffffffffUL;
	}
	check(absSum == MPEG25_ABS_SUM, "sum of |sample|", absSum,
	      MPEG25_ABS_SUM);
	check(hash == MPEG25_HASH, "sample hash", (long)hash,
	      (long)MPEG25_HASH);

	if (failures) {
		printf("mpeg25 decode test: %d failure(s)\n", failures);
		return 1;
	}
	printf("mpeg25 decode test: OK\n");
	return 0;
}
