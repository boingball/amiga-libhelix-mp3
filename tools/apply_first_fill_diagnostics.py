from pathlib import Path

path = Path('amiga_mp3dec.c')
text = path.read_text()

# Add diagnostic stage values beside the existing startup stage definitions.
anchor = '#define GUISTART_FILL_BUFFER_A         200\n'
extra = '''#define GUISTART_FILL_BUFFER_A         200
#define GUISTART_FILL_PLANAR_ENTER     201
#define GUISTART_FILL_PLANAR_READ      202
#define GUISTART_FILL_PLANAR_SYNC      203
#define GUISTART_FILL_PLANAR_DECODE    204
#define GUISTART_FILL_PLANAR_DECODED   205
#define GUISTART_FILL_PLANAR_CONVERT   206
#define GUISTART_FILL_PLANAR_COPIED    207
'''
if 'GUISTART_FILL_PLANAR_ENTER' not in text:
    if anchor not in text:
        raise SystemExit('startup stage anchor not found')
    text = text.replace(anchor, extra, 1)

old = '''static int DecodeStreamFillPlanarS8(DecodeStream *stream, const DecodeOptions *opt,
\tsigned char *left, signed char *right, int maxFrames)
{
\tMP3FrameInfo info;
\tint produced;

\tproduced = 0;
'''
new = '''static int DecodeStreamFillPlanarS8(DecodeStream *stream, const DecodeOptions *opt,
\tsigned char *left, signed char *right, int maxFrames)
{
\tMP3FrameInfo info;
\tint produced;

\tif (gGuiPlaybackStatus.startupStage == GUISTART_FILL_BUFFER_A)
\t\tgGuiPlaybackStatus.startupStage = GUISTART_FILL_PLANAR_ENTER;
\tproduced = 0;
'''
if old in text:
    text = text.replace(old, new, 1)
elif 'GUISTART_FILL_PLANAR_ENTER;' not in text:
    raise SystemExit('planar entry block not found')

old = '''\t\tif (stream->bytesLeft < 2 * MAINBUF_SIZE && !stream->eofReached) {
\t\t\tnRead = FillReadBuffer(stream->readBuf, stream->readPtr,
\t\t\t\tREADBUF_SIZE, stream->bytesLeft, stream->input);
\t\t\tstream->bytesLeft += nRead;
\t\t\tstream->readPtr = stream->readBuf;
\t\t\tif (nRead == 0)
\t\t\t\tstream->eofReached = 1;
\t\t}

\t\toffset = FindValidatedMpegSync(stream->readPtr, stream->bytesLeft);
'''
new = '''\t\tif (stream->bytesLeft < 2 * MAINBUF_SIZE && !stream->eofReached) {
\t\t\tif (gGuiPlaybackStatus.startupStage < GUISTART_FILL_BUFFER_A_DONE)
\t\t\t\tgGuiPlaybackStatus.startupStage = GUISTART_FILL_PLANAR_READ;
\t\t\tnRead = FillReadBuffer(stream->readBuf, stream->readPtr,
\t\t\t\tREADBUF_SIZE, stream->bytesLeft, stream->input);
\t\t\tstream->bytesLeft += nRead;
\t\t\tstream->readPtr = stream->readBuf;
\t\t\tif (nRead == 0)
\t\t\t\tstream->eofReached = 1;
\t\t}

\t\tif (gGuiPlaybackStatus.startupStage < GUISTART_FILL_BUFFER_A_DONE)
\t\t\tgGuiPlaybackStatus.startupStage = GUISTART_FILL_PLANAR_SYNC;
\t\toffset = FindValidatedMpegSync(stream->readPtr, stream->bytesLeft);
'''
# This block occurs in both fill functions; replace the second occurrence (planar).
if 'GUISTART_FILL_PLANAR_READ;' not in text:
    first = text.find(old)
    second = text.find(old, first + 1) if first >= 0 else -1
    if second < 0:
        raise SystemExit('planar read/sync block not found')
    text = text[:second] + new + text[second + len(old):]

old = '''\t\tif (stream->timing) {
\t\t\tt0 = clock();
\t\t\terr = MP3Decode(stream->decoder, &stream->readPtr,
\t\t\t\t&stream->bytesLeft, stream->decodeBuf, 0);
\t\t\tstream->timing->frameDecode += clock() - t0;
\t\t} else {
\t\t\terr = MP3Decode(stream->decoder, &stream->readPtr,
\t\t\t\t&stream->bytesLeft, stream->decodeBuf, 0);
\t\t}
'''
new = '''\t\tif (gGuiPlaybackStatus.startupStage < GUISTART_FILL_BUFFER_A_DONE)
\t\t\tgGuiPlaybackStatus.startupStage = GUISTART_FILL_PLANAR_DECODE;
\t\tif (stream->timing) {
\t\t\tt0 = clock();
\t\t\terr = MP3Decode(stream->decoder, &stream->readPtr,
\t\t\t\t&stream->bytesLeft, stream->decodeBuf, 0);
\t\t\tstream->timing->frameDecode += clock() - t0;
\t\t} else {
\t\t\terr = MP3Decode(stream->decoder, &stream->readPtr,
\t\t\t\t&stream->bytesLeft, stream->decodeBuf, 0);
\t\t}
\t\tif (gGuiPlaybackStatus.startupStage < GUISTART_FILL_BUFFER_A_DONE)
\t\t\tgGuiPlaybackStatus.startupStage = GUISTART_FILL_PLANAR_DECODED;
'''
if 'GUISTART_FILL_PLANAR_DECODE;' not in text:
    first = text.find(old)
    second = text.find(old, first + 1) if first >= 0 else -1
    if second < 0:
        raise SystemExit('planar MP3Decode block not found')
    text = text[:second] + new + text[second + len(old):]

old = '''\t\tMP3GetLastFrameInfo(stream->decoder, &info);
\t\tUpdateFirstFrameStats(stream->stats, &info);
'''
new = '''\t\tif (gGuiPlaybackStatus.startupStage < GUISTART_FILL_BUFFER_A_DONE)
\t\t\tgGuiPlaybackStatus.startupStage = GUISTART_FILL_PLANAR_CONVERT;
\t\tMP3GetLastFrameInfo(stream->decoder, &info);
\t\tUpdateFirstFrameStats(stream->stats, &info);
'''
if 'GUISTART_FILL_PLANAR_CONVERT;' not in text:
    first = text.find(old)
    second = text.find(old, first + 1) if first >= 0 else -1
    if second < 0:
        raise SystemExit('planar conversion block not found')
    text = text[:second] + new + text[second + len(old):]

old = '''\t\tstream->stats->outputSamples += (unsigned long)(frames * 2);
\t\tstream->stats->decodedFrames++;
'''
new = '''\t\tstream->stats->outputSamples += (unsigned long)(frames * 2);
\t\tstream->stats->decodedFrames++;
\t\tif (gGuiPlaybackStatus.startupStage < GUISTART_FILL_BUFFER_A_DONE)
\t\t\tgGuiPlaybackStatus.startupStage = GUISTART_FILL_PLANAR_COPIED;
'''
if old in text:
    text = text.replace(old, new, 1)
elif 'GUISTART_FILL_PLANAR_COPIED;' not in text:
    raise SystemExit('planar copied marker not found')

path.write_text(text)
