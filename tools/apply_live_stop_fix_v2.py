#!/usr/bin/env python3
from pathlib import Path

path = Path("amiga_mp3dec.c")
text = path.read_text(encoding="utf-8")

replacements = [
(
"""#define AMIGA_MONO_AUDIO_SLOTS 3
#define AMIGA_STEREO_AUDIO_SLOTS 2
#define AMIGA_STEREO_DECODE_SLOTS 3
#define AMIGA_AUDIO_PLAYBACK_SLOTS AMIGA_MONO_AUDIO_SLOTS
""",
"""/* Keep no more than two live audio.device writes per channel.  The earlier
 * three-request mono ring can leave Stop blocked while audio.device unwinds the
 * queued writes on real hardware.  Arrays remain sized for the stereo Fast-RAM
 * decode-ahead slot C, but only A/B are submitted to Paula. */
#define AMIGA_MONO_AUDIO_SLOTS 2
#define AMIGA_STEREO_AUDIO_SLOTS 2
#define AMIGA_STEREO_DECODE_SLOTS 3
#define AMIGA_AUDIO_PLAYBACK_SLOTS AMIGA_STEREO_DECODE_SLOTS
""",
1
),
(
"""\tDecodeStreamCopySpill(stream, dest, maxBytes, &produced);
\twhile (produced < maxBytes && !stream->outOfData) {
""",
"""\tDecodeStreamCopySpill(stream, dest, maxBytes, &produced);
\twhile (produced < maxBytes && !stream->outOfData && !gPlaybackInterrupted) {
""",
1
),
(
"""\t\t} else {
\t\t\terr = MP3Decode(stream->decoder, &stream->readPtr,
\t\t\t\t&stream->bytesLeft, stream->decodeBuf, 0);
\t\t}
\t\tif (err) {
""",
"""\t\t} else {
\t\t\terr = MP3Decode(stream->decoder, &stream->readPtr,
\t\t\t\t&stream->bytesLeft, stream->decodeBuf, 0);
\t\t}
\t\tif (gPlaybackInterrupted)
\t\t\tbreak;
\t\tif (err) {
""",
2
),
(
"""\tDecodeStreamCopyPlanarSpill(stream, left, right, maxFrames, &produced);
\twhile (produced < maxFrames && !stream->outOfData) {
""",
"""\tDecodeStreamCopyPlanarSpill(stream, left, right, maxFrames, &produced);
\twhile (produced < maxFrames && !stream->outOfData && !gPlaybackInterrupted) {
""",
1
),
(
"""\t/* No request, device, port, or DMA buffer is destroyed until every write
\t * has either completed or has been aborted and reaped with WaitIO. */
\tfor (i = 0; i < AMIGA_AUDIO_PLAYBACK_SLOTS; i++) {
\t\tfor (ch = 0; ch < 2; ch++) {
\t\t\tif (player->req[i][ch] && player->sent[i][ch]) {
\t\t\t\tint done = CheckIO((struct IORequest *)player->req[i][ch]) != 0;
\t\t\t\tint aborted = 0;
\t\t\t\tAmigaAudioCleanupTrace4(player, "request index=%ld channel=%ld submitted=%ld CheckIO=%ld\\n",
\t\t\t\t\t(unsigned long)i, (unsigned long)ch,
\t\t\t\t\t(unsigned long)player->sent[i][ch], (unsigned long)done);
\t\t\t\tif (!done) {
\t\t\t\t\tAbortIO((struct IORequest *)player->req[i][ch]);
\t\t\t\t\taborted = 1;
\t\t\t\t\tif (status)
\t\t\t\t\t\tstatus->ioAborted++;
\t\t\t\t} else if (status) {
\t\t\t\t\tstatus->ioCompleted++;
\t\t\t\t}
\t\t\t\tAmigaAudioCleanupTrace4(player, "request index=%ld channel=%ld AbortIO issued=%ld\\n",
\t\t\t\t\t(unsigned long)i, (unsigned long)ch, (unsigned long)aborted, 0);
\t\t\t\tWaitIO((struct IORequest *)player->req[i][ch]);
\t\t\t\tAmigaAudioCleanupTrace4(player, "request index=%ld channel=%ld WaitIO completed=1\\n",
\t\t\t\t\t(unsigned long)i, (unsigned long)ch, 0, 0);
\t\t\t\tplayer->sent[i][ch] = 0;
\t\t\t}
\t\t}
\t}
""",
"""\t/* First request cancellation for the entire ring, then reap it in a second
\t * pass.  Waiting one slot at a time lets audio.device advance the next queued
\t * write between AbortIO calls, which is exactly the Stop hang seen with the
\t * old three-request mono ring on real hardware. */
\tfor (i = 0; i < AMIGA_AUDIO_PLAYBACK_SLOTS; i++) {
\t\tfor (ch = 0; ch < 2; ch++) {
\t\t\tif (player->req[i][ch] && player->sent[i][ch]) {
\t\t\t\tint done = CheckIO((struct IORequest *)player->req[i][ch]) != 0;
\t\t\t\tint aborted = 0;
\t\t\t\tAmigaAudioCleanupTrace4(player, "request index=%ld channel=%ld submitted=%ld CheckIO=%ld\\n",
\t\t\t\t\t(unsigned long)i, (unsigned long)ch,
\t\t\t\t\t(unsigned long)player->sent[i][ch], (unsigned long)done);
\t\t\t\tif (!done) {
\t\t\t\t\tAbortIO((struct IORequest *)player->req[i][ch]);
\t\t\t\t\taborted = 1;
\t\t\t\t\tif (status)
\t\t\t\t\t\tstatus->ioAborted++;
\t\t\t\t} else if (status) {
\t\t\t\t\tstatus->ioCompleted++;
\t\t\t\t}
\t\t\t\tAmigaAudioCleanupTrace4(player, "request index=%ld channel=%ld AbortIO issued=%ld\\n",
\t\t\t\t\t(unsigned long)i, (unsigned long)ch, (unsigned long)aborted, 0);
\t\t\t}
\t\t}
\t}
\tfor (i = 0; i < AMIGA_AUDIO_PLAYBACK_SLOTS; i++) {
\t\tfor (ch = 0; ch < 2; ch++) {
\t\t\tif (player->req[i][ch] && player->sent[i][ch]) {
\t\t\t\tWaitIO((struct IORequest *)player->req[i][ch]);
\t\t\t\tAmigaAudioCleanupTrace4(player, "request index=%ld channel=%ld WaitIO completed=1\\n",
\t\t\t\t\t(unsigned long)i, (unsigned long)ch, 0, 0);
\t\t\t\tplayer->sent[i][ch] = 0;
\t\t\t}
\t\t}
\t}
""",
1
),
(
"""\t\tif (got & SIGBREAKF_CTRL_C) {
\t\t\tgPlaybackInterrupted = 1;
\t\t\tAbortIO(req);
\t\t\tbreak;
\t\t}
""",
"""\t\tif (got & SIGBREAKF_CTRL_C) {
\t\t\tgPlaybackInterrupted = 1;
\t\t\tplayer->stopping = 1;
\t\t\tif (!CheckIO(req))
\t\t\t\tAbortIO(req);
\t\t\tbreak;
\t\t}
""",
1
),
(
"""static int AmigaAudioAbortSent(AmigaAudioPlayer *player, int index, int ch)
{
\tstruct IORequest *req;
\tint err;

\treq = (struct IORequest *)player->req[index][ch];
\tif (!req || !player->sent[index][ch])
\t\treturn 0;
\tif (!CheckIO(req))
\t\tAbortIO(req);
\terr = WaitIO(req);
\tif (!err)
\t\terr = player->req[index][ch]->ioa_Request.io_Error;
\tplayer->sent[index][ch] = 0;
\treturn err;
}

static int AmigaAudioAbortOutstanding(AmigaAudioPlayer *player)
{
\tint i;
\tint ch;
\tint err;

\terr = 0;
\tfor (i = 0; i < AMIGA_AUDIO_PLAYBACK_SLOTS; i++) {
\t\tfor (ch = 0; ch < 2; ch++) {
\t\t\tint err2 = AmigaAudioAbortSent(player, i, ch);
\t\t\tif (!err)
\t\t\t\terr = err2;
\t\t}
\t}
\treturn err;
}
""",
"""static int AmigaAudioAbortOutstanding(AmigaAudioPlayer *player)
{
\tint i;
\tint ch;
\tint err;

\tif (!player)
\t\treturn 0;
\tplayer->stopping = 1;
\t/* Cancel every queued request before waiting for any one request. */
\tfor (i = 0; i < AMIGA_AUDIO_PLAYBACK_SLOTS; i++) {
\t\tfor (ch = 0; ch < 2; ch++) {
\t\t\tstruct IORequest *req = (struct IORequest *)player->req[i][ch];
\t\t\tif (req && player->sent[i][ch] && !CheckIO(req))
\t\t\t\tAbortIO(req);
\t\t}
\t}
\terr = 0;
\tfor (i = 0; i < AMIGA_AUDIO_PLAYBACK_SLOTS; i++) {
\t\tfor (ch = 0; ch < 2; ch++) {
\t\t\tstruct IORequest *req = (struct IORequest *)player->req[i][ch];
\t\t\tif (req && player->sent[i][ch]) {
\t\t\t\tint err2 = WaitIO(req);
\t\t\t\tif (!err2)
\t\t\t\t\terr2 = player->req[i][ch]->ioa_Request.io_Error;
\t\t\t\tplayer->sent[i][ch] = 0;
\t\t\t\tif (!err)
\t\t\t\t\terr = err2;
\t\t\t}
\t\t}
\t}
\treturn err;
}
""",
1
),
]

for index, (old, new, expected) in enumerate(replacements, 1):
    count = text.count(old)
    if count != expected:
        raise SystemExit(f"replacement {index}: expected {expected} matches, found {count}")
    text = text.replace(old, new)

path.write_text(text, encoding="utf-8")
